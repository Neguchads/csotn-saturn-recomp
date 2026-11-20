"""Compila/executa oráculos GPU sem SDL_Window, ROM, partida ou save pessoal."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def main():
    runtime = ROOT / 'engine/runtime'
    clang = Path(os.environ['SOTN_CLANG_CL'])
    sdl = Path(os.environ['SDL3_ROOT'])
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
    output = ROOT / 'test-results' / ('gpu-' + str(time.time_ns()))
    output.mkdir(parents=True)
    includes = [runtime / name for name in ('include', 'tests', 'src/host', 'src/video', 'src/machine')]
    includes.append(sdl / 'include')
    groups = {
        'gpu_shader': [ROOT / 'scripts/offscreen/gpu_shader_offscreen.cpp'],
        'fsr': [ROOT / 'scripts/offscreen/fsr_offscreen.cpp'],
        'bug_report': [ROOT / 'scripts/offscreen/bug_report_offscreen.cpp'],
        'fuzz': [ROOT / 'scripts/offscreen/differential_fuzz.cpp', runtime / 'src/video/vdp2.cpp'],
        'vdp2': [runtime / 'tests' / name for name in ('vdp2_gpu_test.cpp', 'vdp2_gpu_compare.cpp', 'vdp2_test.cpp')],
    }
    for name, sources in groups.items():
        command = ('@echo off\ncall "' + vs + '\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\n'
            'if errorlevel 1 exit /b 1\n'
            f'"{clang}" /nologo /std:c++20 /O2 /Gy /EHsc /MT /UNDEBUG '
            + ' '.join(f'/I"{path}"' for path in includes) + ' '
            + ' '.join(f'"{path}"' for path in sources)
            + f' /Fe:"{output / (name + ".exe")}" /link /OPT:REF "{sdl / "lib/x64/SDL3.lib"}"\n')
        script = output / (name + '.cmd')
        script.write_text(command, encoding='utf-8')
        result = subprocess.run(['cmd.exe', '/d', '/c', str(script)], cwd=output,
                                capture_output=True, text=True, timeout=120)
        (output / (name + '-build.log')).write_text(result.stdout + result.stderr, encoding='utf-8')
        if result.returncode:
            raise SystemExit(f'Falha compilando {name}; veja {output}')
    shutil.copy2(sdl / 'lib/x64/SDL3.dll', output / 'SDL3.dll')
    records = []
    commands = [(name, [backend]) for name in ('gpu_shader', 'vdp2', 'fuzz')
                for backend in ('direct3d12', 'vulkan')]
    commands += [('fsr', []), ('bug_report', [str(output / 'synthetic-reports')])]
    for number, (name, arguments) in enumerate(commands):
        result = subprocess.run([str(output / (name + '.exe')), *arguments], cwd=output,
                                capture_output=True, text=True, timeout=120)
        label = name + ('-' + arguments[0] if name != 'bug_report' and arguments else '')
        (output / f'{number}-{name}.log').write_text(result.stdout + result.stderr, encoding='utf-8')
        records.append({'name': label, 'exit': result.returncode})
        (output / 'checks.json').write_text(json.dumps(records, indent=2), encoding='utf-8')
        if result.returncode:
            raise SystemExit(f'Falha executando {label}; veja {output}')
        print('PASS', label, flush=True)
    print(f'Provas: {output}; nenhuma janela ou jogo foi aberto.')


if __name__ == '__main__':
    main()
