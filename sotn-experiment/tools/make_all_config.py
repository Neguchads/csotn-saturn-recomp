"""Prepara um perfil amplo experimental, separado do executável já validado."""
import argparse
import hashlib
import json
import re
from pathlib import Path

import make_config as base


STAGE_BASE = 0x060DC000
# EntityEntry compartilhadas: ZERO noop e GAME cfgrw2. Não são funções locais.
SHARED_ENTRIES = {0x06044348, 0x060854D8}
EXTRA_BASES = {'RICHTER2': (0x060A5000, 17), 'ENDING': (STAGE_BASE, 1)}
FAMILIARS = {'T_BAT', 'T_DEVIL', 'T_DEVIL2', 'T_FAIRY', 'T_FAIRY2', 'T_GHOST', 'T_SWORD'}
# Fins de código confirmados, em offsets do arquivo original.
# STAGE_16: descoberta SH-2 chega a 0xCC10 com as 81 entradas; o YAML estende
# a faixa até 0xD018, mas dados residentes mutáveis aparecem a partir de 0xCF68.
# STAGE_15: código/literais até 0x1418C; o final antigo incluía allocationIndex
# e flags de SaturnSpriteResource, alterados normalmente pelo jogo (crash25/10).
STAGE_CODE_ENDS = {'STAGE_15': 0x14190, 'STAGE_16': 0xCC10,
                   'RSTAGE15': 0x10EFC, 'RSTAGE16': 0xC908, 'WARP': 0x51E0}

# Despachos secundários por entity+0x74: conferir MOV.L da tabela e JSR @r1.
# Somente slots comprovados ausentes; dados adjacentes não viram funções.
SECONDARY_DISPATCH = {
    'BOSS_07': (0x060E5B26, 0x060FB354, {
        9: 0x060E6E24, 19: 0x060E715C, 31: 0x060E7318,
        40: 0x060E7C5C, 42: 0x060F00D0, 43: 0x060F0B64,
        53: 0x060F5C90, 60: 0x060F4130, 62: 0x060E5950}),
    'BOSS_04': (0x060EB340, 0x060F89C8, {
        2: 0x060EBBA8, 16: 0x060F0408, 20: 0x060F2860,
        54: 0x060F06D4, 55: 0x060F32E8}),
    'BOSS_R09': (0x060EA354, 0x060F69D8, {
        2: 0x060EAAE0, 16: 0x060EF340, 54: 0x060EF60C}),
}


def secondary_dispatch_seeds(name, data):
    """Valida no original o consumidor e cada destino documentado antes de semear."""
    if name not in SECONDARY_DISPATCH:
        return set()
    consumer, table, slots = SECONDARY_DISPATCH[name]
    offset = consumer - STAGE_BASE
    if data[offset:offset + 8] != bytes.fromhex('d13b011e410b6483'):
        raise ValueError(f'{name}: consumidor de despacho secundário divergente')
    literal = ((consumer + 4) & ~3) + 0x3B * 4 - STAGE_BASE
    if int.from_bytes(data[literal:literal + 4], 'big') != table:
        raise ValueError(f'{name}: tabela de despacho secundário divergente')
    for slot, expected in slots.items():
        position = table - STAGE_BASE + slot * 4
        callback = int.from_bytes(data[position:position + 4], 'big')
        if callback != expected or not STAGE_BASE + 0x40 <= callback < STAGE_BASE + len(data):
            raise ValueError(f'{name}: callback secundário divergente no slot {slot}')
    return set(slots.values())


def stage_layout(data, address):
    """Segue somente os campos tipados do cabeçalho e dos EntityEntry."""
    def word(pointer):
        offset = pointer - address
        if pointer % 4 or offset < 0 or offset + 4 > len(data):
            raise ValueError(f'Ponteiro de tabela inválido: {pointer:08X}')
        return int.from_bytes(data[offset:offset + 4], 'big')

    def local(pointer):
        return address + 0x40 <= pointer < address + len(data)

    if len(data) < 0x40:
        raise ValueError('Cabeçalho de fase truncado')
    table = word(address)
    word(table)
    boundaries = {word(address + n * 4) for n in range(10) if local(word(address + n * 4))}
    seeds = set()
    shared = set()
    for pointer in range(table, address + len(data) - 3, 4):
        entry = word(pointer)
        if entry == 0:
            if not seeds or not boundaries:
                raise ValueError('Fase sem callbacks locais ou sem fronteira de dados')
            boundary = min(boundaries) - address
            if any(seed >= address + boundary for seed in seeds):
                raise ValueError('Callback depois da primeira estrutura de dados; exige revisão específica')
            return seeds, boundary, sorted(shared)
        if entry in SHARED_ENTRIES:
            shared.add(entry)
            continue
        bank, callback = word(entry), word(entry + 4)
        if callback % 2 or not local(callback):
            raise ValueError(f'Callback não local ou ímpar: {callback:08X}')
        boundaries.add(entry)
        if local(bank):
            boundaries.add(bank)
        seeds.add(callback)
    raise ValueError('Tabela de entidades sem terminador')


def function_header(data, address, count):
    """Armas, jogadores e familiares têm tabelas de callbacks com tamanho tipado."""
    if len(data) < count * 4:
        raise ValueError('Cabeçalho de callbacks truncado')
    values = {int.from_bytes(data[n * 4:n * 4 + 4], 'big') for n in range(count)}
    seeds = {value for value in values if address + count * 4 <= value < address + len(data) and value % 2 == 0}
    if not seeds:
        raise ValueError('Programa sem callbacks locais válidos')
    return seeds


def make_slice(name, data, address, seeds, start, end, source, offset, slices):
    if not 0 <= start < end <= len(data):
        raise ValueError(f'{name}: intervalo inválido {start:X}..{end:X}')
    base.check_seeds(name, address + start, end - start, seeds)
    path = slices / (name + '.bin')
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data[start:end])
    file = path.relative_to(base.ROOT).as_posix() if path.is_relative_to(base.ROOT) else str(path)
    return {'name': name, 'file': file, 'base': address + start, 'source': source,
            'source_offset': offset + start, 'seeds': sorted(seeds), 'callable_entry': True}


def prepare(extract=base.EXTRACT, decomp=base.DECOMP, output=None):
    output = output or base.ROOT / 'build/jp-all'
    slices = output / 'slices'
    # Preservar a configuração e os recortes já comprovados, mas escrever em outra pasta.
    modules = base.module_entries(extract, decomp, slices=slices)
    names = {module['name'] for module in modules}
    metadata = [{'name': module['name'], 'basis': 'existing-profile', 'source': module['source']}
                for module in modules]
    aliases = []
    table_path = decomp / 'src/saturn/zero/stgfiles.c'
    stages = set(re.findall(r'"([A-Z0-9_]+\.PRG)"', table_path.read_text(encoding='utf-8')))
    missing = sorted(name for name in stages if not (extract / name).is_file())
    processed = {module['source'].lstrip('/') for module in modules}
    for path in sorted(extract.glob('*.PRG')):
        name = path.stem
        if name in names or name.startswith('WEAPON'):
            continue
        data = path.read_bytes()
        shared = []
        if path.name in stages:
            address = STAGE_BASE
            seeds, boundary, shared = stage_layout(data, address)
            seeds.update(base.decomp_seeds(decomp / 'asm/saturn' / name.lower()))
            seeds.update(secondary_dispatch_seeds(name, data))
            seeds.add(address + 0x40)
            start = 0x40
            # Limitar a descoberta antes das estruturas tipadas; nunca semear seus ponteiros.
            end = STAGE_CODE_ENDS.get(name) or base.code_end(data[:boundary], address, seeds)
            basis = 'typed-stage-header-and-data-boundary'
        elif name in FAMILIARS or name in EXTRA_BASES:
            address, count = (0x060CF000, 16) if name in FAMILIARS else EXTRA_BASES[name]
            seeds = function_header(data, address, count)
            start = base.code_start(data, min(seeds) - address)
            seeds.add(address + start)
            end = base.code_end(data, address, seeds)
            basis = 'typed-function-header-static-discovery'
        else:
            raise ValueError(f'Programa sem classificação comprovada: {path.name}')
        modules.append(make_slice(name, data, address, seeds, start, end, '/' + path.name, 0, slices))
        metadata.append({'name': name, 'source': '/' + path.name, 'sha256': hashlib.sha256(data).hexdigest(),
                         'code_range': [start, end], 'basis': basis, 'shared_entries': shared,
                         'runtime_identity': 'not_measured'})
        names.add(name)
        processed.add(path.name)
    # Cada contêiner tem 64 registros de seis setores. IDs 20/21 são duplicatas reais.
    for container, address in [('WEAPON0.PRG', 0x060D6000), ('WEAPON1.PRG', 0x060D9000)]:
        packed = (extract / container).read_bytes()
        if len(packed) != 64 * 0x3000:
            raise ValueError(f'{container}: tamanho inesperado')
        seen = {}
        for record in range(64):
            name = f'{Path(container).stem}_{record}'
            offset = record * 0x3000
            data = packed[offset:offset + 0x3000]
            identity = hashlib.sha256(data).hexdigest()
            if identity in seen:
                aliases.append({'name': name, 'alias_of': seen[identity], 'source': '/' + container,
                                'source_offset': offset, 'sha256': identity})
                continue
            seen[identity] = name
            if name in names:
                continue
            seeds = function_header(data, address, 16)
            start = base.code_start(data, min(seeds) - address)
            seeds.add(address + start)
            end = base.code_end(data, address, seeds)
            modules.append(make_slice(name, data, address, seeds, start, end, '/' + container, offset, slices))
            metadata.append({'name': name, 'source': '/' + container, 'source_offset': offset,
                             'sha256': identity, 'code_range': [start, end],
                             'basis': 'typed-weapon-header-static-discovery', 'runtime_identity': 'not_measured'})
        processed.add(container)
    unclassified = sorted(path.name for path in extract.glob('*.PRG') if path.name not in processed)
    if unclassified:
        raise ValueError('Programas não classificados: ' + ', '.join(unclassified))
    report = {'kind': 'experimental-all-programs', 'modules': metadata, 'aliases': aliases,
              'missing_stage_files': missing, 'unclassified_prgs': unclassified,
              'limits': 'Static discovery; new CRC identity and gameplay not measured. Missing files are not fabricated.'}
    return modules, report


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=base.ROOT / 'build/jp-all')
    parser.add_argument('--config', type=Path, default=base.ROOT / 'game-jp-all.toml')
    args = parser.parse_args(argv)
    modules, report = prepare(output=args.output)
    relative = args.output.relative_to(base.ROOT).as_posix()
    text = base.render(modules).replace('build = "build/jp"', f'build = "{relative}"')
    text += '\n[inventory]\nmanifest = "build/jp/analysis/disc.json"\nfiles = "build/jp/extract"\n'
    args.config.write_text(text, encoding='utf-8', newline='\n')
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / 'preparation.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(f'{len(modules)} módulos; {len(report["aliases"])} aliases; ausentes: {report["missing_stage_files"]}', flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
