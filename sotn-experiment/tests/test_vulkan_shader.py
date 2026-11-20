import hashlib
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess


ROOT = Path(__file__).resolve().parents[2]
HOST = ROOT / 'engine/runtime/src/host'
SDL = Path(os.environ.get('SDL3_ROOT', str(ROOT / 'sotn-experiment/build/windows-deps/SDL3-3.4.14')))


def test_spirv_matches_source_and_sdl_bindings():
    source = (HOST / 'sotn_gpu_shader.hlsl').read_text(encoding='utf-8')
    embedded = (HOST / 'sotn_gpu_shader_spirv.h').read_text(encoding='utf-8')
    digest = hashlib.sha256(source.encode('utf-8')).hexdigest()
    assert f'// HLSL SHA256: {digest}\n' in embedded
    binary = bytes(int(value, 16) for value in re.findall(r'0x([0-9a-f]{2})', embedded))
    words = struct.unpack(f'<{len(binary) // 4}I', binary)
    assert words[0] == 0x07230203
    decorations = {}
    inputs = set()
    sampled_image_type = False
    offset = 5
    while offset < len(words):
        count, opcode = words[offset] >> 16, words[offset] & 65535
        assert count > 0 and offset + count <= len(words)
        args = words[offset + 1:offset + count]
        if opcode == 71 and len(args) >= 3:  # OpDecorate.
            decorations.setdefault(args[0], {})[args[1]] = args[2]
        if opcode == 59 and args[2] == 1:  # OpVariable Input.
            inputs.add(args[1])
        if opcode == 27:  # OpTypeSampledImage: SDL exige sampler combinado.
            sampled_image_type = True
        offset += count
    assert sampled_image_type
    descriptors = {(values[34], values[33]) for values in decorations.values() if 34 in values and 33 in values}
    assert descriptors == {(2, 0), (3, 0)}
    assert {decorations[value][30] for value in inputs if 30 in decorations.get(value, {})} == {0, 1}


def test_production_shader_and_hd_scaler_on_real_vulkan(tmp_path):
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    installation = subprocess.check_output([
        str(vswhere), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath',
    ], text=True).strip()
    executable = tmp_path / 'vulkan-shader-test.exe'
    compiler = Path(os.environ.get('SOTN_CLANG_CL', str(ROOT / 'sotn-experiment/build/windows-deps/llvm/bin/clang-cl.exe')))
    script = tmp_path / 'build.cmd'
    script.write_text(
        '@echo off\ncall "' + installation + '\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\n'
        'if errorlevel 1 exit /b 1\n'
        f'"{compiler}" /nologo /std:c++20 /EHsc /MT /W4 /WX /I"{HOST}" /I"{SDL / "include"}" '
        f'"{ROOT / "sotn-experiment/tests/vulkan_shader_test.cpp"}" '
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
    assert 'PASS: real Vulkan GPU CRT/LCD, HD 2x/4x/6x/8x and clean UI' in result.stdout
