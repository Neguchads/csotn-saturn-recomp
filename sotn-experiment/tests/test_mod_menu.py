import os
import re
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[2]
MACHINE = ROOT / 'engine/runtime/src/machine'


def test_dev_presets_offsets_match_saturn_status_layout():
    # Deriva offsets da estrutura Saturn real, sem reutilizar os offsets do preset.
    reference = Path(os.environ.get('SOTN_DECOMP_REFERENCE', str(ROOT / 'referencias/sotn-decomp')))
    symbols = (reference / 'config/saturn/zero_user_syms.txt').read_text(encoding='utf-8')
    base = int(re.search(r'_g_Status\s*=\s*(0x[0-9A-Fa-f]+)', symbols).group(1), 16)
    source = (reference / 'src/saturn/sattypes.h').read_text(encoding='utf-8')
    body = re.search(r'typedef struct\s*\{([^{}]*)\}\s*PlayerStatus;', source, re.S).group(1)
    body = re.sub(r'//[^\n]*|/\*.*?\*/', '', body, flags=re.S)
    offset = 0
    fields = {}
    for kind, name, count in re.findall(r'\b(u8|char|u32|s32)\s+(\w+)(?:\[(\d+)\])?\s*;', body):
        size = 1 if kind in ('u8', 'char') else 4
        offset = (offset + size - 1) // size * size
        fields[name] = base + offset
        offset += size * int(count or 1)
        if name == 'gold':
            break
    assert fields['hp'] == 0x0605C940 and fields['hpMax'] == 0x0605C944
    assert fields['mp'] == 0x0605C950 and fields['mpMax'] == 0x0605C954
    assert fields['hearts'] == 0x0605C948 and fields['heartsMax'] == 0x0605C94C
    assert fields['statsBase'] == 0x0605C958 and fields['gold'] == 0x0605C990
    assert fields['relics'] == 0x0605C6F0
    assert re.search(r'u8\s+relics\[32\]', body)


def test_saturn_original_relic_table_distinguishes_familiars_and_hidden_slot():
    reference = Path(os.environ.get('SOTN_DECOMP_REFERENCE', str(ROOT / 'referencias/sotn-decomp')))
    source = (reference / 'src/saturn/game/cfgjp.c').read_text(encoding='utf-8')
    table = source.split('RelicDesc g_RelicDefs[] = {', 1)[1].split('};', 1)[0]
    kinds = [int(value) for value in re.findall(r',\s*(\d+)\s*\},', table)]
    assert len(kinds) == 32
    assert [i for i, value in enumerate(kinds) if value] == list(range(18, 25))
    original = ROOT / 'sotn-experiment/build/jp/extract/GAME.PRG'
    if original.is_file():
        data = original.read_bytes()
        offset = 0x06080088 - 0x06066000
        actual = [int.from_bytes(data[offset + i * 16 + 12:offset + i * 16 + 16], 'big')
                  for i in range(32)]
        assert actual == kinds
    # Menu SUB original mantém a entrada31 oculta. Não promover por só existir.
    menu = ROOT / 'sotn-experiment/build/jp/extract/SUB.PRG'
    if menu.is_file():
        # ADD #31 seguido de LD8 / AND #254 / ST8 no original limpa FOUND.
        instructions = menu.read_bytes()[0x2BD4:0x2BDE]
        assert instructions == bytes.fromhex('6183711f6010c9fe2100')


def test_production_saturn_mod_parser_and_executor(tmp_path):
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    installation = subprocess.check_output([
        str(vswhere), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath',
    ], text=True).strip()
    executable = tmp_path / 'mod-menu-test.exe'
    compiler = Path(os.environ.get('SOTN_CLANG_CL', str(ROOT / 'sotn-experiment/build/windows-deps/llvm/bin/clang-cl.exe')))
    script = tmp_path / 'build.cmd'
    script.write_text(
        '@echo off\ncall "' + installation + '\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\n'
        'if errorlevel 1 exit /b 1\n'
        f'"{compiler}" /nologo /std:c++20 /EHsc /MT /W4 /I"{MACHINE}" '
        f'"{ROOT / "sotn-experiment/tests/mod_menu_test.cpp"}" /Fo:"{tmp_path / "driver.obj"}" /Fe:"{executable}"\n',
        encoding='utf-8',
    )
    build = subprocess.run(['cmd.exe', '/d', '/c', str(script)], cwd=tmp_path,
                           text=True, capture_output=True, timeout=60)
    assert build.returncode == 0, build.stdout + build.stderr
    result = subprocess.run([str(executable), str(ROOT / 'sotn-experiment/tools/sotn-gameshark.txt')],
                            text=True, capture_output=True, timeout=15)
    assert result.returncode == 0, result.stdout + result.stderr
    assert 'PASS: Saturn mod parser and executor' in result.stdout
