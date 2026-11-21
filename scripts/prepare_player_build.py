"""Build locally from the player's disc; never starts the game or downloads media."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import subprocess
import sys

REFERENCE_URL = 'https://github.com/Xeeynamo/sotn-decomp.git'
REFERENCE_REVISION = '2be55ee17221ee306e330edf2df2d49d48ba47ae'
SPLITTER_REVISION = '68bc84e58de207c92e7ec0b8e887262fdcfdec2f'
EXPECTED_EXECUTABLE = Path('sotn-experiment/build/jp/windows-reference-all/game/saturn.exe')
SPLIT_CONFIGS = (
    ('zero.bin.yaml', 'zero'), ('game.prg.yaml', 'game'),
    ('richter.prg.yaml', 'richter'), ('alucard.prg.yaml', 'alucard'),
    ('maria.prg.yaml', 'maria'), ('t_bat.prg.yaml', 't_bat'),
    ('stage_02.prg.yaml', 'stage_02'), ('stage_15.prg.yaml', 'stage_15'),
    ('stage_16.prg.yaml', 'stage_16'), ('rstage15.prg.yaml', 'rstage15'),
    ('rstage16.prg.yaml', 'rstage16'), ('warp.prg.yaml', 'warp'),
)


class PreparationError(RuntimeError):
    """An actionable preparation failure; no successful build is claimed."""


def validate_identity(product, version):
    if (product.strip(), version.strip()) != ('T-9527G', 'V1.400'):
        raise PreparationError('Expected Saturn Japan T-9527G V1.400; another disc was selected.')


def validate_disc_path(name):
    path = PurePosixPath(name.strip('/'))
    if not path.parts or any(part in {'..', '.'} for part in path.parts) or '\\' in name or ':' in name:
        raise PreparationError('Disc contains an unsafe extraction path.')


def validate_retail_hashes(extract, hash_file):
    rows = []
    seen = set()
    for line in hash_file.read_text(encoding='utf-8-sig').splitlines():
        if not line.strip():
            continue
        fields = line.split()
        if len(fields) != 2 or not re.fullmatch('[0-9a-f]{40}', fields[0]):
            raise PreparationError('Malformed reference SHA-1 manifest.')
        expected, filename = fields
        name = PurePosixPath(filename).name
        if name in seen or not name or '\\' in filename:
            raise PreparationError('Duplicate or invalid reference hash entry.')
        seen.add(name)
        source = extract / name
        if not source.is_file() or source.is_symlink():
            raise PreparationError('Required disc program is missing: ' + name)
        digest = hashlib.sha1(source.read_bytes()).hexdigest()
        if digest != expected:
            raise PreparationError('Disc program does not match the pinned release: ' + name)
        rows.append({'file': name, 'sha1': digest})
    if len(rows) != 12:
        raise PreparationError('The reference must identify exactly 12 retail programs.')
    return rows


def _execute(command, *, cwd, env):
    options = {'creationflags': subprocess.CREATE_NO_WINDOW} if os.name == 'nt' else {}
    with (Path(cwd) / 'player-build.log').open('a', encoding='utf-8') as log:
        subprocess.run(command, cwd=cwd, env=env, check=True, stdout=log,
                       stderr=subprocess.STDOUT, **options)


def _git_head(folder):
    return subprocess.check_output(['git', '-C', str(folder), 'rev-parse', 'HEAD'], text=True).strip()


def pinned_reference(workspace, env, run):
    reference = workspace / 'referencias/sotn-decomp-analise'
    if reference.exists():
        raise PreparationError('Reference workspace already exists. Use a fresh build workspace; existing files are preserved.')
    reference.parent.mkdir(parents=True, exist_ok=True)
    run(['git', 'clone', '--no-checkout', '--filter=blob:none', REFERENCE_URL, str(reference)],
        cwd=workspace, env=env)
    run(['git', 'checkout', '--detach', REFERENCE_REVISION], cwd=reference, env=env)
    if _git_head(reference) != REFERENCE_REVISION:
        raise PreparationError('Reference revision differs from the pinned commit.')
    run(['git', 'submodule', 'update', '--init', '--', 'tools/saturn-splitter'], cwd=reference, env=env)
    if _git_head(reference / 'tools/saturn-splitter') != SPLITTER_REVISION:
        raise PreparationError('Saturn splitter revision differs from the pinned commit.')
    return reference


def _validate_source(workspace, cue):
    required = ('engine/src/saturnrecomp/disc.py', 'sotn-experiment/tools/make_all_config.py',
                'sotn-experiment/tools/generate-jp.py', 'sotn-experiment/tools/build-windows-reference.cmd')
    if not workspace.is_dir() or any(not (workspace / path).is_file() for path in required):
        raise PreparationError('An isolated public source workspace is required.')
    if not cue.is_file() or cue.suffix.lower() != '.cue':
        raise PreparationError('Select an existing .cue file with its referenced tracks beside it.')
    if any(character in str(workspace) for character in '\r\n"%!'):
        raise PreparationError('The build workspace path contains unsupported batch expansion characters.')
    if (workspace / EXPECTED_EXECUTABLE).exists() or (workspace / 'preparation-complete.json').exists():
        raise PreparationError('A previous build exists. Use a fresh build workspace; it will not be overwritten.')


def _validate_disc(workspace, cue):
    spec = importlib.util.spec_from_file_location('_csotn_player_disc', workspace / 'engine/src/saturnrecomp/disc.py')
    if spec is None or spec.loader is None:
        raise PreparationError('Cannot load the local disc reader.')
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    try:
        disc = reader.Disc(str(cue))
    except (reader.DiscError, ValueError) as error:
        raise PreparationError('Cannot read the selected Saturn disc: ' + str(error)) from error
    try:
        validate_identity(disc.ip.product, disc.ip.version)
        for entry in disc.iso.walk():
            validate_disc_path(entry.path)
    finally:
        for handle in disc._fh.values():
            handle.close()


def write_configuration(workspace, reference, extract, output, env, run):
    # Processo separado evita importar um make_config de outro checkout na sessão chamadora.
    script = '''import json,sys
from pathlib import Path
sys.path.insert(0, sys.argv[1])
import make_all_config as all_config
root, reference, extract, output, config = map(Path, sys.argv[2:])
modules, report = all_config.prepare(extract=extract, decomp=reference, output=output)
relative = output.relative_to(root).as_posix()
text = all_config.base.render(modules).replace('build = "build/jp"', 'build = "' + relative + '"')
text += '\\n[inventory]\\nmanifest = "build/jp/analysis/disc.json"\\nfiles = "build/jp/extract"\\n'
config.write_text(text, encoding='utf-8', newline='\\n')
output.mkdir(parents=True, exist_ok=True)
(output/'preparation.json').write_text(json.dumps(report, indent=2)+'\\n', encoding='utf-8')
'''
    experiment = workspace / 'sotn-experiment'
    run([sys.executable, '-c', script, str(experiment / 'tools'), str(experiment),
         str(reference), str(extract), str(output), str(experiment / 'game-jp-all.toml')],
        cwd=workspace, env=env)


def prepare(workspace, cue, env=None, run=None):
    """Return the locally built executable, never launch it. Requires fresh isolated sources."""
    workspace, cue = Path(workspace).resolve(), Path(cue).resolve()
    _validate_source(workspace, cue)
    if os.name != 'nt':
        raise PreparationError('This preparation target currently builds Windows x64 only.')
    environment = os.environ.copy()
    if env is not None:
        environment.update(env)
    environment['PYTHONPATH'] = str(workspace / 'engine/src')
    runner = run or _execute
    try:
        print('progress 0.02 Checking the player disc', flush=True)
        _validate_disc(workspace, cue)
        analysis = workspace / 'sotn-experiment/build/jp/analysis'
        extract = workspace / 'sotn-experiment/build/jp/extract'
        if extract.exists():
            raise PreparationError('Extraction output already exists. Use a fresh build workspace.')
        analysis.mkdir(parents=True, exist_ok=True)
        runner([sys.executable, '-m', 'saturnrecomp.disc', str(cue), '--manifest', str(analysis / 'disc.json')],
               cwd=workspace, env=environment)
        runner([sys.executable, '-m', 'saturnrecomp.disc', str(cue), '--extract', str(extract)],
               cwd=workspace, env=environment)
        print('progress 0.15 Preparing the pinned public reference', flush=True)
        reference = pinned_reference(workspace, environment, runner)
        checked = validate_retail_hashes(extract, reference / 'config/check.saturn.sha')
        shutil.copytree(extract, reference / 'disks/saturn')
        splitter_root = reference / 'tools/saturn-splitter/rust-dis'
        environment['CARGO_TARGET_DIR'] = str(splitter_root / 'target')
        runner(['cargo', 'build', '--release', '--locked', '--manifest-path', str(splitter_root / 'Cargo.toml')],
               cwd=workspace, env=environment)
        splitter = splitter_root / 'target/release/rust-dis.exe'
        if not splitter.is_file():
            raise PreparationError('Cargo did not produce rust-dis.exe.')
        print('progress 0.30 Generating symbols from the player disc', flush=True)
        for configuration, module in SPLIT_CONFIGS:
            target = reference / 'asm/saturn' / module
            if any(target.glob('f_*/*.s')):
                raise PreparationError('Stale assembly output found before splitting: ' + module)
            runner([str(splitter), 'config/saturn/' + configuration], cwd=reference, env=environment)
            if not any(target.glob('f_*/*.s')):
                raise PreparationError('The splitter produced no function assembly for ' + module)
        output = workspace / 'sotn-experiment/build/jp-all'
        write_configuration(workspace, reference, extract, output, environment, runner)
        print('progress 0.45 Translating SH-2 code locally', flush=True)
        runner([sys.executable, str(workspace / 'sotn-experiment/tools/generate-jp.py'),
                str(workspace / 'sotn-experiment/game-jp-all.toml')], cwd=workspace, env=environment)
        environment['SOTN_RECOMP_DIR'] = str(output / 'recomp')
        environment['SATURN_RUNTIME'] = str(workspace / 'engine/runtime')
        compiler = environment.get('SOTN_CLANG_CL')
        if compiler:
            environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
        print('progress 0.65 Building the Windows executable locally', flush=True)
        runner(['cmd.exe', '/d', '/c', str(workspace / 'sotn-experiment/tools/build-windows-reference.cmd')],
               cwd=workspace, env=environment)
        executable = workspace / EXPECTED_EXECUTABLE
        if not executable.is_file() or not executable.with_name('SDL3.dll').is_file():
            raise PreparationError('Build did not produce both saturn.exe and SDL3.dll.')
        record = {'kind': 'player-local-build', 'reference_revision': REFERENCE_REVISION,
                  'splitter_revision': SPLITTER_REVISION, 'checked_programs': checked,
                  'executable': EXPECTED_EXECUTABLE.as_posix(),
                  'sha256': hashlib.sha256(executable.read_bytes()).hexdigest(),
                  'limits': 'Generation/build only. No game or save was opened.'}
        (workspace / 'preparation-complete.json').write_text(json.dumps(record, indent=2) + '\n', encoding='utf-8')
        print('progress 1.0 Local build complete', flush=True)
        return executable
    except (OSError, subprocess.CalledProcessError) as error:
        raise PreparationError('Preparation failed; inspect player-build.log in the workspace/reference. ' + str(error)) from error


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workspace', type=Path, required=True)
    parser.add_argument('--game', type=Path, required=True)
    args = parser.parse_args(argv)
    try:
        prepare(args.workspace, args.game)
    except PreparationError as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
