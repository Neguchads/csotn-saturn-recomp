"""Compila apenas o inicializador próprio e registra a procedência; nunca compila o jogo."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def build(output, vcvars=None):
    if os.name != 'nt':
        raise ValueError('Bootstrap executable requires Windows/MSVC')
    if vcvars is None:
        locator = Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)')) / 'Microsoft Visual Studio/Installer/vswhere.exe'
        installation = subprocess.check_output([str(locator), '-latest', '-products', '*', '-requires',
                                               'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
                                               '-property', 'installationPath'], text=True).strip()
        vcvars = Path(installation) / 'VC/Auxiliary/Build/vcvars64.bat'
    if not vcvars.is_file():
        raise ValueError('Install Visual Studio Build Tools with Desktop development with C++')
    output = output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    source = ROOT / 'scripts/recomplauncher_bootstrap.cpp'
    for path in (vcvars, output, source):
        if any(ch in str(path) for ch in '"%\r\n'):
            raise ValueError('Unsupported shell character in build path')
    executable = output / 'CsotnSetup.exe'
    command = output / 'build-bootstrap.cmd'
    command.write_text(f'@echo off\ncall "{vcvars}" >nul\nif errorlevel 1 exit /b 1\n'
                       f'cl /nologo /std:c++17 /EHsc /MT /W4 /WX /utf-8 "{source}" '
                       f'/Fo:"{output / "bootstrap.obj"}" /Fe:"{executable}"\nexit /b %errorlevel%\n', encoding='utf-8')
    subprocess.run(['cmd.exe', '/d', '/c', str(command)], cwd=output, check=True)
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    canonical = subprocess.check_output(['git', 'show', 'HEAD:scripts/recomplauncher_bootstrap.cpp'], cwd=ROOT)
    if source.read_bytes().replace(b'\r\n', b'\n') != canonical.replace(b'\r\n', b'\n'):
        raise ValueError('Commit bootstrap source before generating package provenance')
    provenance = {'kind': 'csotn-bootstrap', 'sha256': hashlib.sha256(executable.read_bytes()).hexdigest(),
                  'source_sha256': hashlib.sha256(canonical).hexdigest(), 'source_rev': revision}
    (output / 'bootstrap-provenance.json').write_text(json.dumps(provenance, indent=2), encoding='utf-8')
    print(executable)
    return executable


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'test-results/bootstrap')
    parser.add_argument('--vcvars', type=Path)
    args = parser.parse_args()
    build(args.output, args.vcvars)


if __name__ == '__main__':
    main()
