"""Escreve game-jp.toml, o arquivo que o saturn-recomp lê (python -m saturnrecomp.build / learn).

Cada módulo é um programa do disco e o endereço onde o jogo o carrega. Os endereços vêm das chamadas
ReadFileToAddr do sotn-decomp (src/saturn/zero.c e game_3a.c), não de chute. As sementes (endereços
onde começam funções) vêm dos nomes dos arquivos asm/saturn/<módulo>/f_*/f<ENDEREÇO>.s que o build do
sotn-decomp gera; programas sem decomp usam só o ponteiro de entrada (primeira palavra do arquivo).
Só metadados de endereço são lidos: nenhum código do decomp é copiado.

    python tools/make_config.py
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
DECOMP = ROOT.parent / 'referencias' / 'sotn-decomp-analise'
EXTRACT = ROOT / 'build/jp/extract'
CONFIG = ROOT / 'game-jp.toml'
SLICES = ROOT / 'build/jp/slices'

# (nome, arquivo extraído, base, pasta do decomp em asm/saturn ou None)
# Primeira fatia: o caminho de boot até o título. Os outros programas entram quando uma execução
# real chegar neles (o learn para com "cannot place" num endereço sem módulo).
MODULES = [
    ('ZERO', '0.BIN', 0x06004080, 'zero'),       # IP.BIN: 1st read file 0.BIN at 06004080
    ('GAME', 'GAME.PRG', 0x06066000, 'game'),    # game.prg.yaml vram
    ('MOVIE', 'MOVIE.PRG', 0x06066000, None),    # zero.c: ReadFileToAddr("MOVIE.PRG", 0x06066000)
    ('LOGO', 'LOGO.PRG', 0x060A5000, None),      # zero.c: ReadFileToAddr("LOGO.PRG", 0x060A5000)
    ('TITLE', 'TITLE.PRG', 0x060A5000, None),    # zero.c: ReadFileToAddr("TITLE.PRG", 0x060A5000)
    ('OPEN', 'OPEN.PRG', 0x060A5000, None),      # zero.c: ReadFileToAddr("OPEN.PRG", 0x060A5000)
    ('STAFF', 'STAFF.PRG', 0x060A5000, None),    # zero.c: ReadFileToAddr("STAFF.PRG", 0x060A5000)
    # Segunda fatia: o prólogo. Depois do vídeo de abertura o GAME chama func_06064684 =
    # DAT_060A5010 (zero_3.c), um ponteiro do cabeçalho do jogador carregado em 060A5000.
    ('RICHTER', 'RICHTER.PRG', 0x060A5000, 'richter'),   # richter.elf vaddr
    ('ALUCARD', 'ALUCARD.PRG', 0x060A5000, 'alucard'),   # alucard.elf vaddr
    ('MARIA', 'MARIA.PRG', 0x060A5000, 'maria'),         # maria.elf vaddr
    ('WEAPON0_37', 'WEAPON0_37.PRG', 0x060D6000, None), # short sword, weaponId 37 em game/cfgjp.c
    ('WEAPON0_43', 'WEAPON0_43.PRG', 0x060D6000, None), # arma inicial, bloco 43 de WEAPON0.PRG
    ('WEAPON1_52', 'WEAPON1_52.PRG', 0x060D9000, None), # segunda regiao, bloco 52 confirmado na RAM
    ('SUB', 'SUB.PRG', 0x060D6000, None),                # game_3a.c: ReadFileToAddr("SUB.PRG", 0x060D6000)
    # g_StageFileRecords[0x1F] (zero/stgfiles.c) é o ST0 do PS1, o prólogo com o Richter; as fases
    # carregam em g_StageOverlayData = 0x060DC000 (zero_user_syms.txt)
    ('STAGE_00', 'STAGE_00.PRG', 0x060DC000, None),
    ('STAGE_0X', 'STAGE_0X.PRG', 0x060DC000, None), # codigo antes dos dados mutaveis
    ('STAGE_02', 'STAGE_02.PRG', 0x060DC000, None), # laboratorio; zero/stgfiles.c, indice 0x0C

]

# Cada registro de arma ocupa seis setores (0x3000 bytes); zero.c: func_06007824.
# Callback observado na transicao gravada; referencia existente em STAGE_0X.PRG.
# Tabela de callbacks da floresta: 060F5F2C..060F5F7C, passo de 8 bytes.
# As palavras alternadas sao dados; nao trata-las como funcoes.
# Entidade 0x48 do castelo: tabela060F3E60 slot71 -> EntityEntry060FA134,
# banco060FA128, callback060EFFD4; observado no fatal PR0606C38C.
OBSERVED_ENTRIES = {
    # WEAPON0_1/WEAPON1_1 chamam este wrapper sqrt por JSR; zero.c:3779.
    'ZERO': {0x0600EE64},
    'STAGE_0X': {0x060E263C, 0x060DC040, 0x060DC87C, 0x060DC9E0, 0x060DCC64,
                 0x060DCDC4, 0x060DD3C8, 0x060DD968, 0x060DDC30, 0x060DDAA4,
                 0x060DDCD0, 0x060DDFB8, 0x060EFFD4},
}
# Comparacao da RAM antes da transicao: dados modificados em 060F24D8..060F3DDF.
# O recorte termina antes dos dados mutaveis. As tabelas posteriores nao sao codigo.
# WEAPON0_37: a descoberta pelo arquivo inteiro confunde a paleta 060D7034
# com codigo e estende o CRC sobre cores que func_06007B34 modifica ao carregar.
# O recorte confirmado termina apos a ultima instrucao/literal em 060D6DF2.
# STAGE_02: stage_02.prg.yaml delimita codigo/literais ate 0x10C50;
# a descoberta pelos callbacks tipados confirma o mesmo limite.
CODE_RANGES = {
    'STAGE_0X': (0x40, 0x164D8), 'WEAPON0_37': (0x60, 0xDF4),
    'STAGE_02': (0x40, 0x10C50),
}
# Formato confirmado nos cabecalhos de STAGE_00, STAGE_0X e STAGE_02: EntityEntry**.
# Nao generalizar outros programas sem conferir seus dados e a regressao.
TYPED_STAGE_PROGRAMS = {'STAGE_00.PRG', 'STAGE_0X.PRG', 'STAGE_02.PRG'}
SHARED_ENTITY_ENTRIES = {0x06044348}  # g_EntityNoopEntry, zero/stgspr0.c
# zero_3.c::func_0601AF5C copia estes campos para ponteiros do menu.
# +0x10/+0x38/+0x40 ainda são DAT_* no decomp, mas os consumidores SH-2
# SUB:78AE, GAME:73A2C e SUB:9064->9148 os invocam por jsr.
# +0x0C permanece não classificado: não promover pelo valor par/local.
SUB_MENU_FUNCTION_FIELDS = (0x00, 0x04, 0x08, 0x10, 0x14, 0x18, 0x1C, 0x20,
                            0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C, 0x40,
                            0x44, 0x48)

PACKED_PROGRAMS = {
    'WEAPON0_37.PRG': ('WEAPON0.PRG', 37 * 0x3000, 0x3000),
    'WEAPON0_43.PRG': ('WEAPON0.PRG', 43 * 0x3000, 0x3000),
    'WEAPON1_52.PRG': ('WEAPON1.PRG', 52 * 0x3000, 0x3000),
}


def read_program(extract, filename):
    if filename in PACKED_PROGRAMS:
        container, offset, size = PACKED_PROGRAMS[filename]
        path = extract / container
        with path.open('rb') as stream:
            stream.seek(offset)
            data = stream.read(size)
        if len(data) != size:
            raise SystemExit(f'{filename}: bloco de arma truncado')
        return data, '/' + container, offset
    path = extract / filename
    if not path.is_file():
        raise SystemExit(f'{filename}: {path} não existe (rode tools/analyze-jp.py antes)')
    return path.read_bytes(), '/' + filename, 0


def header_seeds(data, base):
    """Primeiros 0x40 bytes são a tabela de callbacks de um overlay de arma."""
    return {value for offset in range(0, min(0x40, len(data)) - 3, 4)
            if (value := int.from_bytes(data[offset:offset + 4], 'big')) % 2 == 0
            and base <= value < base + len(data)}


def sub_menu_seeds(data, base):
    """Exportações SUB tipadas, preservadas antes de recortar fora o cabeçalho."""
    if len(data) < 0x4C:
        raise SystemExit('SUB: cabeçalho de 19 campos truncado')
    seeds = set()
    for offset in SUB_MENU_FUNCTION_FIELDS:
        address = int.from_bytes(data[offset:offset + 4], 'big')
        if address % 2 or not base + 0x4C <= address < base + len(data):
            raise SystemExit(f'SUB: callback inválido no campo +{offset:02X}: {address:08X}')
        seeds.add(address)
    return seeds


def stage_entity_seeds(data, base):
    """Header -> EntityEntry** -> {spriteBank, func}; so o segundo campo e codigo."""
    def word(address):
        offset = address - base
        if address % 4 or offset < 0 or offset + 4 > len(data):
            raise SystemExit(f'Tabela de entidades invalida em {address:08X}')
        return int.from_bytes(data[offset:offset + 4], 'big')

    table = word(base)
    # Ler tambem valida o alinhamento e a faixa do primeiro slot.
    word(table)
    seeds = set()
    for address in range(table, base + len(data) - 3, 4):
        entry = word(address)
        if entry == 0:
            return seeds
        if entry in SHARED_ENTITY_ENTRIES:
            continue
        word(entry)  # spriteBank: validar registro, nunca adicionar como funcao.
        callback = word(entry + 4)
        if callback % 2 or not base <= callback < base + len(data):
            raise SystemExit(f'Callback de entidade invalido em {entry:08X}: {callback:08X}')
        seeds.add(callback)
    raise SystemExit('Tabela de entidades sem terminador NULL')


# O splitter grava sem o zero inicial: f6066CE0.s é a função em 06066CE0.
FUNCTION_FILE = re.compile(r'^f([0-9A-Fa-f]{7,8})\.s$')


def decomp_seeds(asm_dir):
    """Endereços das funções que o splitter do sotn-decomp separou em f<ENDEREÇO>.s."""
    seeds = set()
    for path in asm_dir.glob('f_*/*.s'):
        m = FUNCTION_FILE.match(path.name)
        if m:
            seeds.add(int(m.group(1), 16))
    return seeds


def compiler_helper_seeds(symbol_file):
    """Entradas de deslocamento do compilador compartilham caudas de código.
    O splitter dá nome a algumas funções inteiras, mas as chamadas usam também
    os aliases internos exportados (ex.: ___ashiftrt_r4_10 em 06030E50).
    Só ler essa família de funções; símbolos de dados não são entradas.
    """
    if not symbol_file.is_file():
        return set()
    pattern = re.compile(r'^___ashiftrt_r4_\d+\s*=\s*(0x[0-9A-Fa-f]+);', re.M)
    return {int(address, 16) for address in pattern.findall(symbol_file.read_text())}


def entry_seed(data, base):
    """A primeira palavra de um .PRG é o ponteiro de entrada (zero_3.c: func_06064690 = DAT_06066000).
    None quando ela não aponta para dentro do próprio programa."""
    if len(data) < 4:
        return None
    entry = int.from_bytes(data[:4], 'big')
    return entry if base <= entry < base + len(data) and entry % 2 == 0 else None


def code_start(data, limit):
    """Onde o código de um .PRG começa. O gerador do saturn-recomp sempre decodifica a base do módulo
    como função, mas um .PRG abre com um cabeçalho de ponteiros e dados (TITLE.PRG ainda traz nomes de
    fases), que quebra a geração (Unsupported: .word 0xF04C). Procura o primeiro `mov.l Rm,@-R15`
    (0x2Fm6, o empilhamento do prólogo) depois do primeiro ponteiro, sem passar de `limit`, o menor
    deslocamento que precisa estar dentro do módulo (a entrada e as funções do decomp)."""
    for offset in range(4, min(limit, len(data) - 1), 2):
        if data[offset] == 0x2F and data[offset + 1] & 0x0F == 0x06:
            return offset
    return limit


def code_end(data, base, seeds):
    """Onde o código de um .PRG termina: o fim da última instrução, pool de literais ou tabela de
    switch que a descoberta do saturn-recomp alcança a partir das sementes, alinhado a 4. O runtime
    reconhece o módulo pelo CRC da faixa inteira na RAM, e o fim do arquivo é uma área zerada que o
    ZERO preenche antes de chamar a entrada (TITLE.PRG: 38 bytes em 060B2328-060B2AB3 no VBlank
    1300), então essa área não pode entrar no recorte."""
    from saturnrecomp import sh2
    from saturnrecomp.recomp import discover
    prog = discover.Program(sh2.Image(bytes(data), base), sorted(seeds))
    if not prog.code:
        return len(data)
    last = max(prog.code | prog.data) + 2
    return min(len(data), (last - base + 3) & ~3)


def check_seeds(name, base, size, seeds):
    bad = [s for s in seeds if not base <= s < base + size or s % 2]
    if bad:
        raise SystemExit(f'{name}: sementes fora do módulo ou ímpares: '
                         + ', '.join(f'{s:08X}' for s in sorted(bad)[:5]))


def module_entries(extract=EXTRACT, decomp=DECOMP, modules=MODULES, slices=SLICES):
    """Os módulos do game-jp.toml. Um .PRG vira o recorte só do código (slices/NOME.bin), sem o
    cabeçalho nem as variáveis do fim, com base deslocada pelo mesmo tanto; o runtime reconhece o
    módulo pelo CRC dessa faixa da RAM."""
    out = []
    for name, filename, base, asm_name in modules:
        data, source, packed_offset = read_program(extract, filename)
        if asm_name:
            asm_dir = decomp / 'asm/saturn' / asm_name
            seeds = decomp_seeds(asm_dir)
            if not seeds:
                raise SystemExit(f'{name}: nenhuma função em {asm_dir} (rode o build saturn do sotn-decomp)')
        else:
            seeds = set()
        if filename in TYPED_STAGE_PROGRAMS:
            seeds.update(stage_entity_seeds(data, base))
        elif filename == 'SUB.PRG':
            seeds.update(sub_menu_seeds(data, base))
        elif filename.upper().endswith('.PRG'):
            entry = entry_seed(data, base)
            if entry is None:
                raise SystemExit(f'{name}: ponteiro de entrada fora do programa; base {base:08X} errada?')
            seeds.add(entry)
        if asm_name == 'zero':
            seeds.update(compiler_helper_seeds(decomp / 'config/saturn/zero_syms.gen.txt'))
        if filename in PACKED_PROGRAMS:
            seeds.update(header_seeds(data, base))
        seeds.update(OBSERVED_ENTRIES.get(name, set()))
        if name in CODE_RANGES:
            range_start, range_end = CODE_RANGES[name]
            range_end = range_end or len(data)
            if filename in TYPED_STAGE_PROGRAMS:
                outside = sorted(address for address in seeds
                                 if not base + range_start <= address < base + range_end)
                if outside:
                    raise SystemExit(f'{name}: callbacks tipados fora do recorte: '
                                     + ', '.join(f'{address:08X}' for address in outside[:5]))
            seeds = {address for address in seeds if base + range_start <= address < base + range_end}
            seeds.add(base + range_start)
        check_seeds(name, base, len(data), seeds)
        module: dict = dict(name=name, file=f'build/jp/extract/{filename}', base=base,
                            source=source, seeds=sorted(seeds))
        if filename.upper().endswith('.PRG'):
            start = code_start(data, min(seeds) - base)
            if name in CODE_RANGES:
                start, end = CODE_RANGES[name]
                end = end or len(data)
            else:
                end = code_end(data, base, seeds)
            sliced = slices / f'{name}.bin'
            slices.mkdir(parents=True, exist_ok=True)
            sliced.write_bytes(data[start:end])
            sliced = sliced.relative_to(ROOT) if sliced.is_relative_to(ROOT) else sliced
            # O ZERO chama a entrada de cada .PRG por ponteiro e espera o retorno (zero.c:
            # DAT_06057f34 = func_06064690()). Sem callable_entry, o runtime trata a chamada à base
            # como reinício de programa e para com "the program at ... returned".
            module.update(file=sliced.as_posix(), base=base + start, source_offset=packed_offset + start,
                          callable_entry=True)
        out.append(module)
    return out


def render(modules):
    lines = ['# Gerado por tools/make_config.py; edite a lista MODULES lá, não aqui.',
             '[game]', 'name = "SotN Saturn Japan V1.400 - experimental"', 'build = "build/jp"',
             'seeds = "seeds-jp.json"', '']
    for m in modules:
        lines += ['[[module]]', f'name = "{m["name"]}"', f'file = "{m["file"]}"', f'base = 0x{m["base"]:08X}',
                  f'source = "{m["source"]}"']
        if m.get('source_offset'):
            lines.append(f'source_offset = 0x{m["source_offset"]:X}')
        if m.get('callable_entry'):
            lines.append('callable_entry = true')
        lines += ['seeds = [' + ', '.join(f'0x{s:08X}' for s in m['seeds']) + ']', '']
    return '\n'.join(lines)


def main():
    modules = module_entries()
    CONFIG.write_text(render(modules), encoding='utf-8', newline='\n')
    seeds_file = ROOT / 'seeds-jp.json'
    if not seeds_file.exists():
        seeds_file.write_text('{}\n', encoding='utf-8')
    for m in modules:
        print(f'{m["name"]:6} {m["base"]:08X} {len(m["seeds"]):5} sementes')
    return 0


if __name__ == '__main__':
    sys.exit(main())
