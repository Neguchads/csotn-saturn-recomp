"""Compila HLSL próprio para DXIL e SPIR-V usando o DXC existente no Windows SDK."""
import argparse
import hashlib
from pathlib import Path
import os
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
HOST = ROOT / 'engine/runtime/src/host'


def embedded_header(bytecode, digest, symbol):
    lines = [', '.join(f'0x{value:02x}' for value in bytecode[offset:offset + 16])
             for offset in range(0, len(bytecode), 16)]
    return ('#pragma once\n#include <cstdint>\n'
            '// Gerado por build-gpu-shader.py; alterar o HLSL, não estes bytes.\n'
            f'// HLSL SHA256: {digest}\n'
            f'alignas(4) inline constexpr std::uint8_t {symbol}[] = {{\n    '
            + ',\n    '.join(lines) + '\n};\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dxc', type=Path)
    args = parser.parse_args()
    candidates = sorted((Path(os.environ['ProgramFiles(x86)']) / 'Windows Kits/10/bin').glob('*/x64/dxc.exe'))
    portable = ROOT / 'sotn-experiment/build/windows-deps/dxc-1.9.2609/bin/x64/dxc.exe'
    compiler = args.dxc or (portable if portable.is_file() else candidates[-1] if candidates else None)
    if compiler is None or not compiler.is_file():
        parser.error('DXC não encontrado. Informe --dxc com um executável existente.')
    source = HOST / 'sotn_gpu_shader.hlsl'
    source_text = source.read_text(encoding='utf-8')
    digest = hashlib.sha256(source_text.encode('utf-8')).hexdigest()
    with tempfile.TemporaryDirectory(prefix='sotn-shader-') as temporary:
        binary = Path(temporary) / 'sotn.dxil'
        subprocess.run([str(compiler), '-T', 'ps_6_0', '-E', 'main', '-O3', '-Ges', '-WX',
                        '-Qstrip_debug', '-Fo', str(binary), str(source)], check=True)
        bytecode = binary.read_bytes()
        # SDL Vulkan vincula um combined image sampler por slot, nao dois recursos separados.
        texture = 'Texture2D gameTexture : register(t0, space2);'
        sampler = 'SamplerState gameSampler : register(s0, space2);'
        if source_text.count(texture) != 1 or source_text.count(sampler) != 1:
            parser.error('Contrato de textura/sampler mudou; revisar bindings Vulkan antes de compilar.')
        spirv_source = source_text.replace(texture, '[[vk::combinedImageSampler]] ' + texture)
        spirv_source = spirv_source.replace(sampler, '[[vk::combinedImageSampler]] ' + sampler)
        temporary_source = Path(temporary) / 'sotn-vulkan.hlsl'
        temporary_source.write_text(spirv_source, encoding='utf-8', newline='\n')
        spirv_binary = Path(temporary) / 'sotn.spv'
        subprocess.run([str(compiler), '-spirv', '-fspv-target-env=vulkan1.0', '-T', 'ps_6_0',
                        '-E', 'main', '-O3', '-Ges', '-WX', '-Fo', str(spirv_binary),
                        str(temporary_source)], check=True)
        spirv = spirv_binary.read_bytes()
    # Os dois formatos devem compilar antes de substituir qualquer header.
    (HOST / 'sotn_gpu_shader_dxil.h').write_text(
        embedded_header(bytecode, digest, 'kSotnGpuDxil'), encoding='utf-8', newline='\n')
    (HOST / 'sotn_gpu_shader_spirv.h').write_text(
        embedded_header(spirv, digest, 'kSotnGpuSpirv'), encoding='utf-8', newline='\n')
    print(f'Shader DXIL: {len(bytecode)} bytes; source SHA256 {digest}')
    print(f'Shader SPIR-V: {len(spirv)} bytes; source SHA256 {digest}')


if __name__ == '__main__':
    main()
