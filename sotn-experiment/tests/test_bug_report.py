import json
import os
from pathlib import Path
import shutil
import subprocess

from PIL import Image
from test_vulkan_shader import ROOT, HOST, SDL


def test_bug_report_captures_real_gpu_and_handles_missing_images(tmp_path):
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    installation = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
    exe = tmp_path / 'bug-report-test.exe'
    compiler = Path(os.environ.get('SOTN_CLANG_CL', str(ROOT / 'sotn-experiment/build/windows-deps/llvm/bin/clang-cl.exe')))
    script = tmp_path / 'build.cmd'
    script.write_text('@echo off\ncall "' + installation + '\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\n'
        f'"{compiler}" /nologo /std:c++20 /EHsc /MT /W4 /WX /I"{HOST}" /I"{SDL / "include"}" '
        f'"{ROOT / "sotn-experiment/tests/bug_report_test.cpp"}" /Fo:"{tmp_path / "driver.obj"}" /Fe:"{exe}" '
        f'/link /LIBPATH:"{SDL / "lib/x64"}" SDL3.lib\n', encoding='utf-8')
    build = subprocess.run(['cmd.exe', '/d', '/c', str(script)], cwd=tmp_path,
                           text=True, capture_output=True, timeout=60)
    assert build.returncode == 0, build.stdout + build.stderr
    shutil.copy2(SDL / 'lib/x64/SDL3.dll', tmp_path / 'SDL3.dll')
    output = tmp_path / 'relatorios de bugs'
    result = subprocess.run([str(exe), str(output)], text=True, capture_output=True, timeout=50)
    assert result.returncode == 0, result.stdout + result.stderr
    for backend in ('direct3d12', 'vulkan'):
        reports = [json.loads(path.read_text()) for path in (output / backend / 'bugs').glob('*/report.json')]
        assert len(reports) == 2
        complete = next(report for report in reports if report['rendered_saved'])
        assert complete['gpu_backend'] == backend
        assert complete['escaped'] == 'quote" slash\\ line\n'
        assert complete['input_copied'] == 'true' and complete['run.log_copied'] == 'true'
        assert complete['session_metadata_copied'] == 'true'
        missing = next(report for report in reports if not report['rendered_saved'])
        assert 'rendered_capture_error' in missing and 'original_capture_error' in missing
        folder = next(path.parent for path in (output / backend / 'bugs').glob('*/report.json')
                      if json.loads(path.read_text())['rendered_saved'])
        with Image.open(folder / 'original.png') as original, Image.open(folder / 'rendered.png') as rendered:
            assert original.size == (8, 8) and rendered.size == (64, 64)
            assert original.convert('RGB').getpixel((0, 0)) == (128, 128, 128)
            assert len(set(rendered.convert('RGB').tobytes())) > 1
        assert not list(folder.glob('*.bin'))
        assert json.loads((folder / 'session.json').read_text())['executable_sha256'] == 'test-hash'
        assert json.loads((folder / 'relato.json').read_text()) == {'titulo': 'Titulo "bug"', 'descricao': 'Descricao\nlinha2'}
