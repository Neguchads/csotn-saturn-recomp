import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
RUNTIME = ROOT / 'engine/runtime'
SDL = Path(os.environ.get('SDL3_ROOT', str(ROOT / 'sotn-experiment/build/windows-deps/SDL3-3.4.14')))


def test_real_sdl_gamepad_feeds_production_dev_input_helpers(tmp_path):
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    installation = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
    compiler = Path(os.environ.get('SOTN_CLANG_CL', str(ROOT / 'sotn-experiment/build/windows-deps/llvm/bin/clang-cl.exe')))
    executable = tmp_path / 'dev-host-input.exe'
    includes = [RUNTIME / 'include', SDL / 'include']
    includes += [RUNTIME / 'src' / name for name in ('host', 'machine', 'video', 'sound', 'core')]
    source = ROOT / 'sotn-experiment/tests/dev_host_input_test.cpp'
    script = tmp_path / 'build.cmd'
    script.write_text('@echo off\ncall "' + installation + '\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\n'
        f'"{compiler}" /nologo /std:c++20 /EHsc /MT /W4 /WX '
        + ' '.join(f'/I"{include}"' for include in includes)
        + f' "{source}" /Fe:"{executable}" /link /LIBPATH:"{SDL / "lib/x64"}" SDL3.lib\n',
        encoding='utf-8')
    build = subprocess.run(['cmd.exe', '/d', '/c', str(script)], cwd=tmp_path,
                           capture_output=True, text=True, timeout=90)
    assert build.returncode == 0, build.stdout + build.stderr
    shutil.copy2(SDL / 'lib/x64/SDL3.dll', tmp_path / 'SDL3.dll')
    result = subprocess.run([str(executable)], cwd=tmp_path, capture_output=True,
                            text=True, timeout=15)
    assert result.returncode == 0, result.stdout + result.stderr
    assert 'PASS SDL virtual gamepad and production helpers' in result.stdout
