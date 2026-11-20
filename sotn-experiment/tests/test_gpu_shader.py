import os
import hashlib
from pathlib import Path
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[2]
HOST = ROOT / 'engine/runtime/src/host'
SDL = Path(os.environ.get('SDL3_ROOT', str(ROOT / 'sotn-experiment/build/windows-deps/SDL3-3.4.14')))


def test_embedded_gpu_shader_matches_hlsl_source():
    source = (HOST / 'sotn_gpu_shader.hlsl').read_text(encoding='utf-8')
    digest = hashlib.sha256(source.encode('utf-8')).hexdigest()
    embedded = (HOST / 'sotn_gpu_shader_dxil.h').read_text(encoding='utf-8')
    assert f'// HLSL SHA256: {digest}\n' in embedded
    assert '0x44, 0x58, 0x42, 0x43' in embedded  # Contêiner DXIL começa com DXBC.


def test_production_gpu_shaders_on_real_d3d12_renderer(tmp_path):
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    installation = subprocess.check_output([
        str(vswhere), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath',
    ], text=True).strip()
    executable = tmp_path / 'gpu-shader-test.exe'
    compiler = Path(os.environ.get('SOTN_CLANG_CL', str(ROOT / 'sotn-experiment/build/windows-deps/llvm/bin/clang-cl.exe')))
    script = tmp_path / 'build.cmd'
    script.write_text(
        '@echo off\ncall "' + installation + '\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\n'
        'if errorlevel 1 exit /b 1\n'
        f'"{compiler}" /nologo /std:c++20 /EHsc /MT /W4 /WX /I"{HOST}" /I"{SDL / "include"}" '
        f'"{ROOT / "sotn-experiment/tests/gpu_shader_test.cpp"}" '
        f'/Fo:"{tmp_path / "driver.obj"}" /Fe:"{executable}" '
        f'/link /LIBPATH:"{SDL / "lib/x64"}" SDL3.lib\n',
        encoding='utf-8',
    )
    build = subprocess.run(['cmd.exe', '/d', '/c', str(script)], cwd=tmp_path,
                           text=True, capture_output=True, timeout=60)
    assert build.returncode == 0, build.stdout + build.stderr
    shutil.copy2(SDL / 'lib/x64/SDL3.dll', tmp_path / 'SDL3.dll')
    result = subprocess.run([str(executable)], text=True, capture_output=True, timeout=30)
    assert result.returncode == 0, result.stdout + result.stderr
    assert 'PASS: real D3D12 GPU CRT/LCD pixels' in result.stdout
