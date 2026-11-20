import os
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[2]
HOST = ROOT / 'engine/runtime/src/host'


def test_production_video_and_gamepad_settings(tmp_path):
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    installation = subprocess.check_output([
        str(vswhere), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath',
    ], text=True).strip()
    executable = tmp_path / 'settings-video-test.exe'
    compiler = Path(os.environ.get('SOTN_CLANG_CL', str(ROOT / 'sotn-experiment/build/windows-deps/llvm/bin/clang-cl.exe')))
    script = tmp_path / 'build.cmd'
    script.write_text(
        '@echo off\ncall "' + installation + '\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\n'
        'if errorlevel 1 exit /b 1\n'
        f'"{compiler}" /nologo /std:c++20 /EHsc /MT /W4 /I"{HOST}" /c '
        f'"{ROOT / "sotn-experiment/tests/settings_video_test.cpp"}" /Fo:"{tmp_path / "driver.obj"}"\n'
        'if errorlevel 1 exit /b 1\n'
        f'"{compiler}" /nologo /std:c++20 /EHsc /MT /W4 /I"{HOST}" /c '
        f'"{HOST / "settings.cpp"}" /Fo:"{tmp_path / "settings.obj"}"\n'
        'if errorlevel 1 exit /b 1\n'
        f'"{compiler}" /nologo "{tmp_path / "driver.obj"}" "{tmp_path / "settings.obj"}" /Fe:"{executable}"\n',
        encoding='utf-8',
    )
    build = subprocess.run(['cmd.exe', '/d', '/c', str(script)], cwd=tmp_path,
                           text=True, capture_output=True, timeout=60)
    assert build.returncode == 0, build.stdout + build.stderr
    result = subprocess.run([str(executable)], text=True, capture_output=True, timeout=15)
    assert result.returncode == 0, result.stdout + result.stderr
    assert 'PASS: production video and gamepad settings' in result.stdout
