import os
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import make_all_config

ROOT = Path(__file__).resolve().parents[2]
RUNTIME = ROOT / 'engine/runtime'


def test_reload_reidentifies_a_shared_callback_before_dispatch(tmp_path):
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    installation = subprocess.check_output([
        str(vswhere), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath',
    ], text=True).strip()
    compiler = Path(os.environ.get('SOTN_CLANG_CL', str(ROOT / 'sotn-experiment/build/windows-deps/llvm/bin/clang-cl.exe')))
    executable = tmp_path / 'overlay-residency.exe'
    script = tmp_path / 'build.cmd'
    script.write_text(
        '@echo off\ncall "' + installation + '\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\n'
        'if errorlevel 1 exit /b 1\n'
        f'"{compiler}" /nologo /std:c++20 /EHsc /MT /W4 /I"{RUNTIME / "include"}" '
        f'"{ROOT / "sotn-experiment/tests/overlay_residency_test.cpp"}" '
        f'"{RUNTIME / "src/core/core.cpp"}" /Fe:"{executable}"\n', encoding='utf-8')
    build = subprocess.run(['cmd.exe', '/d', '/c', str(script)], cwd=tmp_path,
                           capture_output=True, text=True, timeout=60)
    assert build.returncode == 0, build.stdout + build.stderr
    result = subprocess.run([str(executable)], cwd=tmp_path, capture_output=True, text=True, timeout=15)
    assert result.returncode == 0, result.stdout + result.stderr
    assert 'PASS: residency reload' in result.stdout
    original = ROOT / 'sotn-experiment/build/jp/extract/STAGE_MA.PRG'
    captured = ROOT / 'logs-e-bugs/partidas/20261025-082225-302-20784/session-001/overlay-failure-fatal.bin'
    if original.is_file() and captured.is_file():
        real = subprocess.run([str(executable), str(original), str(captured)], cwd=tmp_path,
                              capture_output=True, text=True, timeout=15)
        assert real.returncode == 0, real.stdout + real.stderr
        assert 'captured Maria failure dispatches STAGE_03' in real.stdout
    original = ROOT / 'sotn-experiment/build/jp/extract/STAGE_15.PRG'
    captured = ROOT / 'logs-e-bugs/partidas/20261025-142750-774-22156/session-001/overlay-failure-fatal.bin'
    if original.is_file() and captured.is_file():
        fixed = subprocess.run([str(executable), str(original), str(captured),
            str(make_all_config.STAGE_CODE_ENDS['STAGE_15'])], cwd=tmp_path,
            capture_output=True, text=True, timeout=15)
        assert fixed.returncode == 0, fixed.stdout + fixed.stderr
        assert 'captured STAGE_15 rejects old range, dispatches fixed identity and refuses corrupted instruction' in fixed.stdout
    original = ROOT / 'sotn-experiment/build/jp/extract/ENDING.PRG'
    captured = ROOT / 'logs-e-bugs/partidas/20261025-151836-199-27532/session-001/overlay-failure-fatal.bin'
    if original.is_file() and captured.is_file():
        ending = subprocess.run([str(executable), str(original), str(captured), 'ending', 'offset20'],
            cwd=tmp_path, capture_output=True, text=True, timeout=15)
        assert ending.returncode == 0, ending.stdout + ending.stderr
        assert 'captured ENDING at offset20 dispatches and corrupted code stays refused' in ending.stdout
