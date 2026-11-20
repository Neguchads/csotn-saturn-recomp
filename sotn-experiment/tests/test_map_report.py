import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
RUNTIME = ROOT / 'engine/runtime'
SDL = Path(os.environ.get('SDL3_ROOT', str(ROOT / 'sotn-experiment/build/windows-deps/SDL3-3.4.14')))


def test_live_map_and_annotation_form(tmp_path):
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    installation = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
    compiler = Path(os.environ.get('SOTN_CLANG_CL', str(ROOT / 'sotn-experiment/build/windows-deps/llvm/bin/clang-cl.exe')))
    executable = tmp_path / 'map-report.exe'
    # Dois marcadores sintéticos exercitam a política sem publicar o blueprint do jogo.
    (tmp_path / 'sotn_map_markers.local.h').write_text(
        'inline constexpr Marker kMarkers[] = {{false,7,30,1},{false,15,38,2}};\n', encoding='utf-8')
    includes = [tmp_path, RUNTIME / 'include', SDL / 'include']
    includes += [RUNTIME / 'src' / name for name in ('host', 'machine', 'video', 'sound', 'core')]
    source = ROOT / 'sotn-experiment/tests/map_report_test.cpp'
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
    assert "PASS: live map data" in result.stdout
