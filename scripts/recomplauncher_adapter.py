"""Preparação local e execução explícita; --check nunca inicia o jogo."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from prepare_player_build import PreparationError, prepare

SOURCE = Path(__file__).resolve().parents[1]
EXCLUDED = {'.git', '.venv', '__pycache__', 'test-results', 'build', 'referencias', 'roms', 'recompdata'}


def source_files(source):
    if (source / '.git').exists():
        names = subprocess.check_output(['git', 'ls-files', '-z'], cwd=source).decode().split('\0')
        files = [source / name for name in names if name]
    else:
        files = [path for path in source.rglob('*') if path.is_file()
                 and not EXCLUDED.intersection(path.relative_to(source).parts)]
    for path in sorted(files):
        if path.is_symlink() or not path.resolve().is_relative_to(source.resolve()):
            raise ValueError('Source links outside the package are not allowed')
        yield path


def source_identity(source):
    digest = hashlib.sha256()
    files = list(source_files(source))
    for path in files:
        digest.update(path.relative_to(source).as_posix().encode() + b'\0')
        digest.update(path.read_bytes())
    return digest.hexdigest(), files


def check_tools(environment=None):
    env = dict(os.environ if environment is None else environment)
    errors = []
    if sys.version_info < (3, 11):
        errors.append('Python 3.11 or later is required')
    if os.name != 'nt':
        errors.append('The developer preview prepares Windows x64 builds only')
    for tool in ('git', 'cargo', 'cmake', 'ninja'):
        if not shutil.which(tool, path=env.get('PATH', '')):
            errors.append(f'{tool} is missing from PATH')
    compiler = env.get('SOTN_CLANG_CL') or shutil.which('clang-cl', path=env.get('PATH', ''))
    if not compiler or not Path(compiler).is_file():
        errors.append('Install LLVM clang-cl or set SOTN_CLANG_CL')
    else:
        env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
    sdl = Path(env.get('SDL3_ROOT', '__missing_sdl__'))
    if not (sdl / 'lib/x64/SDL3.dll').is_file():
        errors.append('Set SDL3_ROOT to the SDL3 3.4.14 Visual C development package')
    locator = Path(env.get('ProgramFiles(x86)', 'C:/Program Files (x86)')) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    vcvars = env.get('VCVARS64')
    if not vcvars and locator.is_file():
        installation = subprocess.check_output([str(locator), '-latest', '-products', '*', '-requires',
                                               'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
                                               '-property', 'installationPath'], text=True).strip()
        if installation:
            vcvars = str(Path(installation) / 'VC/Auxiliary/Build/vcvars64.bat')
    if not vcvars or not Path(vcvars).is_file():
        errors.append('Install Visual Studio Build Tools with Desktop development with C++')
    else:
        env['VCVARS64'] = vcvars
    return errors, env


def atomic_json(path, content):
    path.parent.mkdir(parents=True, exist_ok=True)
    temp = path.with_suffix('.tmp')
    temp.write_text(json.dumps(content, indent=2), encoding='utf-8')
    os.replace(temp, path)


def prepare_player(game, data, source=SOURCE):
    if not game.is_file() or game.suffix.lower() != '.cue':
        raise ValueError('Select the original Japanese T-9527G V1.400 .cue, with its BIN tracks beside it')
    errors, environment = check_tools()
    if errors:
        raise ValueError('\n'.join(errors))
    identity, files = source_identity(source)
    attempts = data / 'build'
    attempts.mkdir(parents=True, exist_ok=True)
    workspace = Path(tempfile.mkdtemp(prefix='work-', dir=attempts)) / 'source'
    print('progress 0.05 Copying public build sources', flush=True)
    for path in files:
        target = workspace / path.relative_to(source)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(path.read_bytes())
    executable = prepare(workspace, game.resolve(), env=environment)
    destination = data / 'game'
    destination.mkdir(parents=True, exist_ok=True)
    for name in ('saturn.exe', 'SDL3.dll'):
        target = destination / name
        pending = destination / (name + '.pending')
        shutil.copyfile(executable.parent / name, pending)
        os.replace(pending, target)
    atomic_json(data / 'build/ready.json', {'format': 1, 'source_sha256': identity,
                                          'cue': str(game.resolve()), 'executable': 'game/saturn.exe',
                                          'executable_sha256': hashlib.sha256((destination / 'saturn.exe').read_bytes()).hexdigest()})
    print('progress 1.0 Local compilation complete; the game was not started', flush=True)


def play_command(data, source=SOURCE):
    record = json.loads((data / 'build/ready.json').read_text(encoding='utf-8'))
    identity, _ = source_identity(source)
    if record.get('source_sha256') != identity:
        raise ValueError('Public sources changed; run preparation again before playing')
    executable = data / 'game/saturn.exe'
    if not executable.is_file() or hashlib.sha256(executable.read_bytes()).hexdigest() != record.get('executable_sha256'):
        raise ValueError('Prepared executable is missing or changed; run preparation again')
    cue = Path(record['cue'])
    if not cue.is_file():
        raise ValueError('Original CUE/BIN media is still required at runtime; select it again')
    (data / 'saves').mkdir(parents=True, exist_ok=True)
    (data / 'logs').mkdir(parents=True, exist_ok=True)
    return [str(executable), '--cue', str(cue), '--save', str(data / 'saves/backup.bin'),
            '--settings', str(data / 'settings.ini'), '--out', str(data / 'logs')]


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--check', action='store_true')
    mode.add_argument('--prepare', action='store_true')
    mode.add_argument('--play', action='store_true')
    parser.add_argument('--game', type=Path)
    parser.add_argument('--data', type=Path, default=Path(os.environ.get('RECOMPLAUNCHER_DATA', 'recompdata')))
    args = parser.parse_args(argv)
    try:
        if args.check:
            errors, _ = check_tools()
            print(json.dumps({'ready': not errors, 'requirements': errors}, indent=2))
            return 2 if errors else 0
        data = args.data.resolve()
        if args.prepare:
            if args.game is None:
                raise ValueError('--game is required for preparation')
            prepare_player(args.game, data)
            return 0
        return subprocess.run(play_command(data), cwd=data).returncode
    except (ValueError, OSError, subprocess.CalledProcessError, PreparationError) as error:
        print(f'Preparation failed: {error}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
