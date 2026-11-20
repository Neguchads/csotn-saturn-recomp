from pathlib import Path
import shutil
import subprocess
from test_vulkan_shader import ROOT, HOST, SDL
import os
import hashlib
import json


def test_fsr_vendor_and_embedded_shaders_match_sources():
    vendor = HOST.parent.parent / 'third_party/fsr1'
    manifest = json.loads((vendor / 'manifest.json').read_text(encoding='utf-8-sig'))
    assert manifest['license'] == 'MIT'
    assert manifest['revision'] == 'a21ffb8f6c13233ba336352bdff293894c706575'
    for entry in manifest['files']:
        assert entry['upstreamMatches']
        assert hashlib.sha256((vendor / entry['path']).read_bytes()).hexdigest() == entry['sha256']
    digest = hashlib.sha256((HOST / 'sotn_fsr.hlsl').read_text(encoding='utf-8').encode('utf-8'))
    for name in ('ffx_a.h','ffx_fsr1.h'):
        digest.update((vendor / name).read_bytes())
    for format in ('dxil','spirv'):
        assert f'// Source/vendor SHA256: {digest.hexdigest()}' in (HOST / f'sotn_fsr_{format}.h').read_text()


def test_fsr1_real_easu_rcas_d3d12_vulkan(tmp_path):
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    installation = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
    exe = tmp_path / 'fsr-test.exe'
    script = tmp_path / 'build.cmd'
    compiler = Path(os.environ.get('SOTN_CLANG_CL', str(ROOT / 'sotn-experiment/build/windows-deps/llvm/bin/clang-cl.exe')))
    script.write_text('@echo off\ncall "' + installation + '\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\n'
        f'"{compiler}" /nologo /std:c++20 /EHsc /MT /W4 /WX /I"{HOST}" /I"{SDL / "include"}" '
        f'"{ROOT / "sotn-experiment/tests/fsr_test.cpp"}" /Fo:"{tmp_path / "driver.obj"}" /Fe:"{exe}" '
        f'/link /LIBPATH:"{SDL / "lib/x64"}" SDL3.lib\n', encoding='utf-8')
    build = subprocess.run(['cmd.exe','/d','/c',str(script)],cwd=tmp_path,text=True,capture_output=True,timeout=60)
    assert build.returncode == 0, build.stdout + build.stderr
    shutil.copy2(SDL / 'lib/x64/SDL3.dll',tmp_path / 'SDL3.dll')
    result = subprocess.run([str(exe)],text=True,capture_output=True,timeout=40)
    assert result.returncode == 0, result.stdout + result.stderr
    assert 'PASS: FSR1 EASU RCAS pixels on vulkan' in result.stdout
    assert 'PASS: FSR1 EASU RCAS pixels on direct3d12' in result.stdout
