import os
from pathlib import Path
import re
import subprocess


ROOT = Path(__file__).resolve().parents[2]
VIDEO = ROOT / 'engine/runtime/src/video'


def test_production_opt_in_video_profiler(tmp_path):
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    installation = subprocess.check_output([
        str(vswhere), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath',
    ], text=True).strip()
    executable = tmp_path / 'video-perf-test.exe'
    compiler = Path(os.environ.get('SOTN_CLANG_CL', str(ROOT / 'sotn-experiment/build/windows-deps/llvm/bin/clang-cl.exe')))
    script = tmp_path / 'build.cmd'
    script.write_text(
        '@echo off\ncall "' + installation + '\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\n'
        'if errorlevel 1 exit /b 1\n'
        f'"{compiler}" /nologo /std:c++20 /EHsc /MT /W4 /WX /I"{VIDEO}" '
        f'"{ROOT / "sotn-experiment/tests/video_perf_test.cpp"}" '
        f'/Fo:"{tmp_path / "driver.obj"}" /Fe:"{executable}"\n',
        encoding='utf-8',
    )
    build = subprocess.run(['cmd.exe', '/d', '/c', str(script)], cwd=tmp_path,
                           text=True, capture_output=True, timeout=60)
    assert build.returncode == 0, build.stdout + build.stderr
    for value in (None, '', '0'):
        environment = os.environ.copy()
        environment.pop('SOTN_VIDEO_PROFILE', None)
        if value is not None:
            environment['SOTN_VIDEO_PROFILE'] = value
        result = subprocess.run([str(executable)], env=environment, text=True,
                                capture_output=True, timeout=15)
        assert result.returncode == 0, result.stdout + result.stderr
        assert result.stderr == ''
    environment = os.environ.copy()
    environment['SOTN_VIDEO_PROFILE'] = '1'
    result = subprocess.run([str(executable), 'enabled'], env=environment, text=True,
                            capture_output=True, timeout=15)
    assert result.returncode == 0, result.stdout + result.stderr
    lines = result.stderr.splitlines()
    assert len(lines) == 2
    assert 'vb=300' in lines[0] and 'size=320x224' in lines[0]
    assert 'vb=301' in lines[1] and 'size=640x480' in lines[1]
    assert float(re.search(r'vb_s=([\d.]+)', lines[0]).group(1)) > 0
    for stage in ('vdp1', 'vdp2', 'upload', 'draw', 'present', 'devices', 'sound', 'cd', 'slave', 'pace'):
        values = re.search(rf'{stage}\(count=(\d+),avg_ms=([\d.]+),max_ms=([\d.]+)\)', lines[0])
        assert values, lines[0]
        count, average, maximum = values.groups()
        assert int(count) == 3
        assert 0 < float(average) <= float(maximum)
    assert 'vdp1(count=0,' in lines[1]
    assert 'vdp2(count=1,' in lines[1]
    assert 'present(count=0,' in lines[1]
