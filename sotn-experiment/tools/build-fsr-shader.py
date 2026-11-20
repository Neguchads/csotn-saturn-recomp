"""Compila os dois passes FSR1 oficiais em DXIL e SPIR-V com o DXC local."""
import argparse
import hashlib
from pathlib import Path
import os
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
HOST = ROOT / 'engine/runtime/src/host'
VENDOR = HOST.parent.parent / 'third_party/fsr1'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dxc', type=Path)
    args = parser.parse_args()
    portable = ROOT / 'sotn-experiment/build/windows-deps/dxc-1.9.2609/bin/x64/dxc.exe'
    candidates = sorted((Path(os.environ['ProgramFiles(x86)']) / 'Windows Kits/10/bin').glob('*/x64/dxc.exe'))
    compiler = args.dxc or (portable if portable.is_file() else candidates[-1] if candidates else None)
    if compiler is None or not compiler.is_file():
        parser.error('DXC não encontrado. Informe --dxc com suporte SPIR-V.')
    source = HOST / 'sotn_fsr.hlsl'
    text = source.read_text(encoding='utf-8')
    digest = hashlib.sha256(text.encode('utf-8'))
    for path in (VENDOR / 'ffx_a.h', VENDOR / 'ffx_fsr1.h'):
        digest.update(path.read_bytes())
    fingerprint = digest.hexdigest()
    with tempfile.TemporaryDirectory(prefix='sotn-fsr-') as temporary:
        temp = Path(temporary)
        for output_format in ('dxil', 'spirv'):
            compiled = []
            temporary_source = text
            if output_format == 'spirv':
                temporary_source = temporary_source.replace(
                    'Texture2D gameTexture', '[[vk::combinedImageSampler]] Texture2D gameTexture').replace(
                    'SamplerState gameSampler', '[[vk::combinedImageSampler]] SamplerState gameSampler')
            # Includes absolutos só na entrada temporária, para manter fonte e fornecedor intactos.
            temporary_source = temporary_source.replace('../../third_party/fsr1/', VENDOR.as_posix() + '/')
            entry = temp / 'fsr.hlsl'
            entry.write_text(temporary_source, encoding='utf-8')
            for name, definition in (('Easu', 'SOTN_EASU=1'), ('Rcas', 'SOTN_RCAS=1')):
                binary = temp / f'{name}.{output_format}'
                command = [str(compiler), '-T', 'ps_6_0', '-E', 'main', '-HV', '2018', '-O3', '-Ges', '-WX',
                           '-D', definition, '-Qstrip_debug', '-Fo', str(binary), str(entry)]
                if output_format == 'spirv':
                    command.extend(['-spirv', '-fspv-target-env=vulkan1.0'])
                subprocess.run(command, check=True)
                bytecode = binary.read_bytes()
                rows = [', '.join(f'0x{value:02x}' for value in bytecode[offset:offset + 16])
                        for offset in range(0, len(bytecode), 16)]
                compiled.append(f'inline constexpr std::uint8_t kSotnFsr{name}{output_format.title()}[] = {{\n    '
                                + ',\n    '.join(rows) + '\n};\n')
                print(f'FSR1 {name} {output_format}: {len(bytecode)} bytes')
            header = ('#pragma once\n#include <cstdint>\n'
                      '// Gerado por build-fsr-shader.py a partir de HLSL e headers AMD oficiais.\n'
                      f'// Source/vendor SHA256: {fingerprint}\n' + ''.join(compiled))
            (HOST / f'sotn_fsr_{output_format}.h').write_text(header, encoding='utf-8', newline='\n')


if __name__ == '__main__':
    main()
