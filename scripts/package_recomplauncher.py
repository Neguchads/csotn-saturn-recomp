"""Empacota apenas fontes Git e, opcionalmente, o bootstrap próprio comprovado."""
import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import struct
import subprocess
import zipfile

from check_repository import inspect

ROOT = Path(__file__).resolve().parents[1]
BOOTSTRAP_SOURCE = 'scripts/recomplauncher_bootstrap.cpp'
BOOTSTRAP_NAME = 'CsotnSetup.exe'
BOOTSTRAP_MARKER = b'CSOTN_SETUP_BOOTSTRAP_V1'
FIXED_TIME = (1980, 1, 1, 0, 0, 0)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def validate_path(name):
    """Rejeita nomes perigosos mesmo quando o Git permite armazená-los."""
    path = PurePosixPath(name)
    reserved = {'CON', 'PRN', 'AUX', 'NUL'} | {f'{prefix}{number}' for prefix in ('COM', 'LPT') for number in range(1, 10)}
    if (not name or path.is_absolute() or '\\' in name or ':' in name
            or any(ord(char) < 32 for char in name)
            or any(part in {'', '.', '..'} or part.endswith((' ', '.'))
                   or part.split('.')[0].upper() in reserved for part in name.split('/'))):
        raise ValueError('Caminho Git inválido para distribuição')
    return path


def git(repository, *args):
    return subprocess.check_output(['git', *args], cwd=repository)


def committed_files(repository):
    revision = git(repository, 'rev-parse', 'HEAD').decode().strip()
    if subprocess.run(['git', 'diff', '--quiet', 'HEAD', '--'], cwd=repository).returncode:
        raise ValueError('Commitar alterações rastreadas antes de empacotar')
    files = {}
    seen = set()
    for record in git(repository, 'ls-tree', '-r', '-z', revision).split(b'\0'):
        if not record:
            continue
        metadata, raw_name = record.split(b'\t', 1)
        mode, kind, blob = metadata.decode().split()
        name = raw_name.decode('utf-8')
        validate_path(name)
        if mode not in {'100644', '100755'} or kind != 'blob':
            raise ValueError('Symlinks/submódulos não são entradas de distribuição: ' + name)
        if name.casefold() in seen:
            raise ValueError('Colisão de caminhos no Windows: ' + name)
        seen.add(name.casefold())
        data = git(repository, 'cat-file', 'blob', blob)
        # O profile novo tem a mesma inspeção de texto/segredos de um JSON público.
        problems = inspect('docs/recomplauncher.json' if name == 'recomplauncher.json' else name, data)
        if problems:
            raise ValueError(name + ': ' + '; '.join(problems))
        files[name] = data
    if 'LICENSE' not in files or 'recomplauncher.json' not in files:
        raise ValueError('LICENSE e recomplauncher.json devem estar no commit')
    profile = json.loads(files['recomplauncher.json'].decode('utf-8-sig'))
    if not isinstance(profile, dict) or profile.get('format') != 1:
        raise ValueError('Profile RecompLauncher deve declarar format:1')
    return revision, files


def verified_bootstrap(path, provenance_path, revision, sources):
    if path.name != BOOTSTRAP_NAME or path.is_symlink() or provenance_path.is_symlink():
        raise ValueError('Somente o bootstrap próprio CsotnSetup.exe é permitido')
    data = path.read_bytes()
    provenance = json.loads(provenance_path.read_text(encoding='utf-8-sig'))
    source = sources.get(BOOTSTRAP_SOURCE)
    if (not isinstance(provenance, dict) or source is None
            or provenance.get('kind') != 'csotn-bootstrap'
            or provenance.get('sha256') != sha256(data)
            or provenance.get('source_sha256') != sha256(source)
            or provenance.get('source_rev') != revision):
        raise ValueError('Proveniência do bootstrap não corresponde ao executável/fonte/revisão')
    offset = struct.unpack_from('<I', data, 0x3C)[0] if len(data) >= 64 else 0
    if (data[:2] != b'MZ' or offset < 64 or offset + 6 > len(data)
            or data[offset:offset + 4] != b'PE\0\0'
            or data[offset + 4:offset + 6] != b'\x64\x86'
            or BOOTSTRAP_MARKER not in data):
        raise ValueError('Bootstrap não é o PE x64 identificado esperado')
    return data, provenance


def build_package(repository, output, bootstrap=None, bootstrap_provenance=None, version=None):
    repository = Path(repository).resolve()
    output = Path(output).resolve()
    if output.suffix.lower() != '.zip':
        raise ValueError('A saída deve ser um arquivo ZIP')
    if bool(bootstrap) != bool(bootstrap_provenance):
        raise ValueError('Bootstrap e proveniência devem ser fornecidos juntos')
    revision, sources = committed_files(repository)
    version = version or revision
    if not isinstance(version, str) or not version.strip() or any(ord(char) < 32 for char in version):
        raise ValueError('Versão do pacote inválida')
    entries = {'source/' + name: data for name, data in sources.items()}
    provenance = None
    if bootstrap:
        data, provenance = verified_bootstrap(Path(bootstrap), Path(bootstrap_provenance), revision, sources)
        entries[BOOTSTRAP_NAME] = data
        profile = json.loads(sources['recomplauncher.json'].decode('utf-8-sig'))
        prepare = profile.setdefault('prepare', {})
        if not isinstance(prepare, dict):
            raise ValueError('prepare deve ser um objeto')
        prepare['version'] = version
        entries['recomplauncher.json'] = (json.dumps(profile, indent=2) + '\n').encode()
    manifest = {
        'format': 1,
        'kind': 'developer-bootstrap' if bootstrap else 'source-only',
        'installable': bool(bootstrap),
        'ready_to_play': False,
        'source_rev': revision,
        'version': version,
        'external_prerequisites': ['Python 3.11+', 'Windows C/C++ build toolchain; see source/docs/SETUP.md'],
        'bootstrap_provenance': provenance,
        'files': [{'path': name, 'sha256': sha256(data), 'size': len(data)}
                  for name, data in sorted(entries.items())],
    }
    entries['package-manifest.json'] = (json.dumps(manifest, sort_keys=True, indent=2) + '\n').encode()
    archive = io.BytesIO()
    with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_STORED) as stream:
        for name, data in sorted(entries.items()):
            info = zipfile.ZipInfo(name, FIXED_TIME)
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            stream.writestr(info, data)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(archive.getvalue())
    return manifest


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=ROOT)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--bootstrap', type=Path)
    parser.add_argument('--bootstrap-provenance', type=Path)
    parser.add_argument('--version', help='Tag da release; padrão: revisão Git')
    args = parser.parse_args(argv)
    try:
        result = build_package(args.repo, args.output, args.bootstrap, args.bootstrap_provenance, args.version)
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        parser.exit(1, str(error) + '\n')
    print(f'{result["kind"]}: {args.output}; not a ready-to-play game release')


if __name__ == '__main__':
    main()
