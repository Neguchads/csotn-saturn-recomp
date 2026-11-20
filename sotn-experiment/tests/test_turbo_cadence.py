from pathlib import Path
import os
import subprocess

ROOT=Path(__file__).resolve().parents[2]
HOST=ROOT/'engine/runtime/src/host'


def test_turbo_presentation_tracks_wall_clock_when_requested_speed_is_not_reached(tmp_path):
    vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
    vs=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
    compiler=Path(os.environ.get('SOTN_CLANG_CL',str(ROOT/'sotn-experiment/build/windows-deps/llvm/bin/clang-cl.exe')))
    cmd=tmp_path/'build.cmd';exe=tmp_path/'cadence.exe'
    cmd.write_text(f'@echo off\ncall "{vs}/VC/Auxiliary/Build/vcvars64.bat" >nul\n'
        f'"{compiler}" /nologo /std:c++20 /O2 /EHsc /MT /W4 /WX /I"{HOST}" '
        f'"{ROOT/"sotn-experiment/tests/turbo_cadence_test.cpp"}" /Fe:"{exe}"\n')
    result=subprocess.run(['cmd.exe','/d','/c',str(cmd)],cwd=tmp_path,capture_output=True,text=True,timeout=60)
    assert result.returncode==0,result.stdout+result.stderr
    result=subprocess.run([str(exe)],cwd=tmp_path,capture_output=True,text=True,timeout=15)
    assert result.returncode==0,result.stdout+result.stderr
    assert 'PASS: adaptive turbo presentation' in result.stdout
    assert 'legacy_fps=10.400 adaptive_fps=59.' in result.stdout
