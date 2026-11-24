"""Confere o conteúdo preparado para publicação, sem executar jogo ou enviar arquivos."""
import ast
from pathlib import Path, PurePosixPath
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
TEXT_EXTENSIONS = {'.py', '.cpp', '.c', '.h', '.hpp', '.inc', '.cmake', '.md', '.txt', '.hlsl',
                   '.json', '.toml', '.lock', '.sh', '.cmd', '.ps1', '.manifest', '.yml', '.yaml'}
ROOT_FILES = {'.gitignore', '.gitattributes', '.gitleaks.toml', 'AGENTS.md', 'README.md', 'ROADMAP.md', 'HANDOFF.md',
              'LICENSE', 'THIRD_PARTY.md', 'requirements-dev.txt', 'requirements-ci.txt', 'Jogar-SotN-Windows.cmd',
              'Jogar-SotN-Amplo.cmd', 'Recompilar-SotN.cmd', 'recomplauncher.json'}
FONTS = {'engine/runtime/third_party/inter/Inter-Regular.ttf',
         'engine/runtime/third_party/inter/Inter-SemiBold.ttf'}
EXTENSIONLESS_SOURCES = {'engine/runtime/third_party/musashi/softfloat/softfloat-macros',
                        'engine/runtime/third_party/musashi/softfloat/softfloat-specialize'}
SECRET = re.compile(r'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----|'
                    r'gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{40,}')


def allowed(name):
    path = PurePosixPath(name)
    if path.is_absolute() or '..' in path.parts or '\\' in name:
        return False
    forbidden = {'.git', '.venv', '__pycache__', 'build', '.pytest_cache', 'node_modules',
                 'roms', 'referencias', 'validacao', 'logs-e-bugs', 'test-results'}
    if any(part in forbidden or part.startswith('.env') for part in path.parts):
        return False
    if '.local.' in path.name or path.name.startswith(('game-', 'seeds-')):
        return False
    if name in ROOT_FILES or name in FONTS or name in EXTENSIONLESS_SOURCES:
        return True
    if name in {'.github/dependabot.yml', '.github/workflows/ci.yml',
                '.github/workflows/workflow-security.yml', '.github/workflows/codeql.yml',
                '.github/workflows/supply-chain.yml', '.github/workflows/advanced-quality.yml',
                '.github/advanced/fuzz.py', '.github/advanced/benchmark.py'}:
        return True
    if len(path.parts) == 3 and path.parts[:2] == ('.codex', 'agents') and path.suffix == '.toml':
        return True
    if not path.parts or path.parts[0] not in {'engine', 'scripts', 'docs', 'sotn-experiment'}:
        return False
    return path.suffix in TEXT_EXTENSIONS or path.name in {'LICENSE', 'VERSION'}


def inspect(name, data):
    if not allowed(name):
        return ['caminho fora da seleção pública']
    if name in FONTS:
        return [] if data[:4] in (b'\x00\x01\x00\x00', b'OTTO') else ['fonte inválida']
    if b'\0' in data:
        return ['binário fora das duas fontes licenciadas']
    try:
        text = data.decode('utf-8-sig')
    except UnicodeDecodeError:
        return ['texto não UTF-8']
    problems = []
    if SECRET.search(text):
        problems.append('possível segredo; valor omitido')
    if name.endswith('.py'):
        try:
            ast.parse(text, filename=name)
        except SyntaxError:
            problems.append('sintaxe Python inválida')
    if name == 'engine/runtime/src/host/sotn_map_markers.h':
        if 'blueprint original Saturn JP local' in text or re.search(r'\{(?:false|true),\s*\d', text):
            problems.append('tabela privada de mapa não pode ser enviada')
    return problems


def main():
    names = subprocess.check_output(['git', 'ls-files', '--cached', '-z'], cwd=ROOT).decode().split('\0')
    failures = []
    for name in filter(None, names):
        data = subprocess.check_output(['git', 'show', ':' + name], cwd=ROOT)
        failures.extend(f'{name}: {problem}' for problem in inspect(name, data))
    if failures:
        raise SystemExit('\n'.join(failures))
    print('PASS: índice público, fronteiras de dados, sintaxe Python e padrões de credenciais')


if __name__ == '__main__':
    main()
