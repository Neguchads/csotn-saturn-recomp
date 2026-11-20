"""Exercita o launcher real com filho de teste; nunca inicia o executável do jogo."""
from pathlib import Path
import json
import os
import shutil
import subprocess
import pytest

ROOT = Path(__file__).resolve().parents[2]


@pytest.mark.parametrize('backend,reference,exit_code', [
    (None, False, 0),
    ('direct3d12', False, 0), ('vulkan', False, 0),
    ('direct3d12', True, 0), ('direct3d12', False, 27),
])
def test_gpu_launcher_forwards_flags_and_child_exit_without_opening_game(tmp_path, backend, reference, exit_code):
    tools = tmp_path / 'project with spaces/sotn-experiment/tools'
    tools.mkdir(parents=True)
    wrapper = tools / 'play-windows-gpu.ps1'
    shutil.copy2(ROOT / 'sotn-experiment/tools/play-windows-gpu.ps1', wrapper)
    (tools / 'play-windows.ps1').write_text('''param([string]$Executable, [switch]$VideoProfile)
@{executable=$Executable;profile=$VideoProfile.IsPresent;gpu=$env:SOTN_VDP2_GPU;backend=$env:SOTN_VDP2_GPU_BACKEND} |
    ConvertTo-Json | Set-Content -LiteralPath $env:SOTN_WRAPPER_CAPTURE -Encoding UTF8
exit ([int]$env:SOTN_WRAPPER_EXIT)
''', encoding='utf-8-sig')
    executable = tmp_path / 'unused game.exe'
    executable.write_bytes(b'not executable; child launcher never starts it')
    sentinel = tmp_path / 'personal-save-fixture.bin'
    sentinel.write_bytes(b'unchanged')
    capture = tmp_path / 'capture.json'
    environment = os.environ.copy()
    environment['SOTN_WRAPPER_CAPTURE'] = str(capture)
    environment['SOTN_WRAPPER_EXIT'] = str(exit_code)
    environment['SOTN_VDP2_GPU'] = 'previous'
    environment['SOTN_VDP2_GPU_BACKEND'] = 'previous'
    shell = Path(os.environ['SystemRoot']) / 'System32/WindowsPowerShell/v1.0/powershell.exe'
    command = [str(shell), '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', str(wrapper),
               '-Executable', str(executable), '-VideoProfile']
    if backend is not None:
        command.extend(['-Backend', backend])
    if reference:
        command.append('-CpuReference')
    result = subprocess.run(command, env=environment, capture_output=True, text=True, timeout=30)
    assert result.returncode == exit_code, result.stdout + result.stderr
    recorded = json.loads(capture.read_text(encoding='utf-8-sig'))
    assert recorded == {'executable': str(executable), 'profile': True,
                        'gpu': '0' if reference else '1', 'backend': backend or 'vulkan'}
    assert sentinel.read_bytes() == b'unchanged'
