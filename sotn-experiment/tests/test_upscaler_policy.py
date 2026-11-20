import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
HOST = ROOT / 'engine/runtime/src/host'


def test_production_upscaler_failure_policy_without_window(tmp_path):
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    installation = subprocess.check_output([
        str(vswhere), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath',
    ], text=True).strip()
    compiler = Path(os.environ.get('SOTN_CLANG_CL', str(ROOT / 'sotn-experiment/build/windows-deps/llvm/bin/clang-cl.exe')))
    executable = tmp_path / 'upscaler-policy.exe'
    script = tmp_path / 'build.cmd'
    script.write_text(
        '@echo off\ncall "' + installation + '\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\n'
        'if errorlevel 1 exit /b 1\n'
        f'"{compiler}" /nologo /std:c++20 /EHsc /MT /W4 /WX /I"{HOST}" '
        f'"{ROOT / "sotn-experiment/tests/upscaler_policy_test.cpp"}" '
        f'/Fo:"{tmp_path / "driver.obj"}" /Fe:"{executable}"\n', encoding='utf-8')
    result = subprocess.run(['cmd.exe', '/d', '/c', str(script)], cwd=tmp_path,
                            capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
    result = subprocess.run([str(executable)], cwd=tmp_path, capture_output=True, text=True, timeout=15)
    assert result.returncode == 0, result.stdout + result.stderr
    assert 'PASS: FSR priority preserves HD preferences' in result.stdout
