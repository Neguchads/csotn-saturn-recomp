import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import make_config  # noqa: E402


def test_main_initializes_local_seeds_without_overwriting_learned_entries(tmp_path, monkeypatch):
    monkeypatch.setattr(make_config, 'ROOT', tmp_path)
    monkeypatch.setattr(make_config, 'CONFIG', tmp_path / 'game-jp.toml')
    monkeypatch.setattr(make_config, 'module_entries', lambda: [])
    assert make_config.main() == 0
    seeds = tmp_path / 'seeds-jp.json'
    assert seeds.read_text() == '{}\n'
    seeds.write_text('{"ZERO": [100]}\n')
    assert make_config.main() == 0
    assert seeds.read_text() == '{"ZERO": [100]}\n'


def test_entry_seed_reads_first_word_inside_program():
    data = bytes.fromhex('06066D14') + bytes(0x2000)
    assert make_config.entry_seed(data, 0x06066000) == 0x06066D14


def test_entry_seed_rejects_pointer_outside_program():
    # MOVIE.PRG lido com a base de TITLE: a entrada cai fora e a base é recusada
    data = bytes.fromhex('06066D14') + bytes(0x2000)
    assert make_config.entry_seed(data, 0x060A5000) is None


def test_entry_seed_rejects_odd_pointer():
    data = bytes.fromhex('06066D15') + bytes(0x2000)
    assert make_config.entry_seed(data, 0x06066000) is None


def test_decomp_seeds_reads_function_files_only(tmp_path):
    (tmp_path / 'f_match').mkdir()
    (tmp_path / 'f_nonmat').mkdir()
    (tmp_path / 'data').mkdir()
    (tmp_path / 'f_match' / 'f6066CE0.s').write_text('')
    (tmp_path / 'f_nonmat' / 'f06066D20.s').write_text('')
    (tmp_path / 'f_nonmat' / 'notes.s').write_text('')
    (tmp_path / 'data' / 'f06070000.s').write_text('')
    assert make_config.decomp_seeds(tmp_path) == {0x06066CE0, 0x06066D20}


def test_check_seeds_rejects_out_of_range():
    with pytest.raises(SystemExit):
        make_config.check_seeds('X', 0x06066000, 0x100, {0x06066100})


def test_code_start_skips_header_to_first_push():
    # MOVIE.PRG: entrada + 7 palavras de dados, depois 2F86 (mov.l r8,@-r15)
    data = bytes.fromhex('06066D14 F04C4100 DC4C4100 C44C4100 B44C4100 A04C4100 804C4100 5C4C4100'
                         '2F862F96')
    assert make_config.code_start(data, 0xD14) == 0x20


def test_code_start_skips_strings():
    # TITLE.PRG: depois do cabeçalho vêm nomes de fases em ASCII, sem 0x2F
    data = (bytes.fromhex('060A5578') + bytes(4) + b'RSTAGE16  \0\0' + b'SHIN.DRA  \0\0'
            + bytes.fromhex('2FE64F22'))
    assert make_config.code_start(data, 0x578) == 32


def test_code_start_never_passes_limit():
    data = bytes.fromhex('06066010') + bytes(0x20) + bytes.fromhex('2F86')
    assert make_config.code_start(data, 0x10) == 0x10


RTS_NOP = bytes.fromhex('000B0009')


def test_code_end_stops_after_code_and_literal_pool():
    # mov.l @(4,PC),r0 lê o literal em base+8; depois rts/nop e a área zerada de variáveis
    data = bytes.fromhex('D001') + RTS_NOP + bytes(2) + bytes.fromhex('12345678') + bytes(0x40)
    assert make_config.code_end(data, 0x060A5000, {0x060A5000}) == 0xC


def test_code_end_keeps_whole_file_without_code():
    assert make_config.code_end(bytes(0x40), 0x060A5000, set()) == 0x40


def test_module_entries_slices_prg_from_code(tmp_path):
    extract = tmp_path / 'extract'
    extract.mkdir()
    movie = bytes.fromhex('06066010') + bytes(0xC) + bytes.fromhex('2F86') + RTS_NOP + bytes(0x40)
    (extract / 'MOVIE.PRG').write_bytes(movie)
    asm = tmp_path / 'decomp' / 'asm/saturn/game/f_match'
    asm.mkdir(parents=True)
    (asm / 'f6066020.s').write_text('')
    game = bytes.fromhex('06066030') + bytes(0x1C) + RTS_NOP + bytes(0xC) + RTS_NOP + bytes(0x40)
    (extract / 'GAME.PRG').write_bytes(game)
    slices = tmp_path / 'slices'
    mods = make_config.module_entries(extract, tmp_path / 'decomp', [
        ('MOVIE', 'MOVIE.PRG', 0x06066000, None),
        ('GAME', 'GAME.PRG', 0x06066000, 'game'),
    ], slices=slices)
    assert mods[0]['seeds'] == [0x06066010]
    assert mods[0]['source'] == '/MOVIE.PRG'
    assert mods[0]['base'] == 0x06066010 and mods[0]['source_offset'] == 0x10
    assert mods[0]['callable_entry'] is True
    # recorte do prólogo até o fim do rts/nop: os zeros do fim ficam de fora
    assert (slices / 'MOVIE.bin').read_bytes() == movie[0x10:0x18]
    assert mods[1]['seeds'] == [0x06066020, 0x06066030]
    # sem prólogo antes da primeira função do decomp: o recorte começa nela
    assert mods[1]['base'] == 0x06066020
    assert (slices / 'GAME.bin').read_bytes() == game[0x20:0x34]


def test_module_entries_keeps_boot_program_whole(tmp_path):
    extract = tmp_path / 'extract'
    extract.mkdir()
    (extract / '0.BIN').write_bytes(bytes(0xAE20))
    asm = tmp_path / 'decomp' / 'asm/saturn/zero/f_match'
    asm.mkdir(parents=True)
    (asm / 'f60040A0.s').write_text('')
    mods = make_config.module_entries(extract, tmp_path / 'decomp', [('ZERO', '0.BIN', 0x06004080, 'zero')],
                                      slices=tmp_path / 'slices')
    assert mods[0]['base'] == 0x06004080
    assert mods[0]['file'].endswith('extract/0.BIN')
    assert 'source_offset' not in mods[0]
    assert 'callable_entry' not in mods[0]
    assert 0x0600EE64 in mods[0]['seeds']


def test_module_entries_stops_when_decomp_has_no_functions(tmp_path):
    extract = tmp_path / 'extract'
    extract.mkdir()
    (extract / '0.BIN').write_bytes(bytes(0x40))
    with pytest.raises(SystemExit):
        make_config.module_entries(extract, tmp_path, [('ZERO', '0.BIN', 0x06004080, 'zero')])


def test_render_is_loadable_toml():
    import tomllib
    text = make_config.render([dict(name='GAME', file='build/jp/slices/GAME.bin', base=0x06066000,
                                    source='/GAME.PRG', seeds=[0x06066040], source_offset=0x40,
                                    callable_entry=True)])
    parsed = tomllib.loads(text)
    assert parsed['module'][0]['callable_entry'] is True
    assert parsed['module'][0]['source_offset'] == 0x40
    assert parsed['module'][0]['base'] == 0x06066000
    assert parsed['module'][0]['seeds'] == [0x06066040]
    assert parsed['game']['seeds'] == 'seeds-jp.json'


def test_compiler_helper_seeds_keeps_fallthrough_shift_entrypoints(tmp_path):
    symbols = tmp_path / 'zero_syms.gen.txt'
    symbols.write_text('___ashiftrt_r4_10 = 0x06030E50;\n'
                       '___ashiftrt_r4_9 = 0x06030E52;\n'
                       '___ashiftrt_r4_32 = 0x06030E18;\n'
                       '___ashiftrt_r4_31 = 0x06030E18;\n'
                       'some_data = 0x06030E54;\n')
    assert make_config.compiler_helper_seeds(symbols) == {0x06030E18, 0x06030E50, 0x06030E52}


def test_module_entries_includes_compiler_aliases_for_zero(tmp_path):
    extract = tmp_path / 'extract'
    extract.mkdir()
    (extract / '0.BIN').write_bytes(bytes(0xAE20))
    decomp = tmp_path / 'decomp'
    asm = decomp / 'asm/saturn/zero/f_nonmat'
    asm.mkdir(parents=True)
    (asm / 'f6004080.s').write_text('')
    symbols = decomp / 'config/saturn/zero_syms.gen.txt'
    symbols.parent.mkdir(parents=True)
    symbols.write_text('___ashiftrt_r4_10 = 0x06004090;\n')
    mods = make_config.module_entries(extract, decomp, [('ZERO', '0.BIN', 0x06004080, 'zero')])
    assert mods[0]['seeds'] == [0x06004080, 0x06004090, 0x0600EE64]


def test_read_program_extracts_only_registered_packed_block(tmp_path, monkeypatch):
    monkeypatch.setattr(make_config, 'PACKED_PROGRAMS', {'WEAPON0_43.PRG': ('WEAPON0.PRG', 8, 8)})
    (tmp_path / 'WEAPON0.PRG').write_bytes(b'previous' + b'weapon43' + b'nextone!')
    data, source, offset = make_config.read_program(tmp_path, 'WEAPON0_43.PRG')
    assert data == b'weapon43'
    assert source == '/WEAPON0.PRG'
    assert offset == 8


def test_read_program_rejects_truncated_packed_block(tmp_path, monkeypatch):
    monkeypatch.setattr(make_config, 'PACKED_PROGRAMS', {'WEAPON0_43.PRG': ('WEAPON0.PRG', 8, 8)})
    (tmp_path / 'WEAPON0.PRG').write_bytes(b'too short')
    with pytest.raises(SystemExit):
        make_config.read_program(tmp_path, 'WEAPON0_43.PRG')


def test_header_seeds_keeps_exported_callbacks_and_rejects_data():
    data = bytearray(0x200)
    for offset, address in [(0, 0x060D6140), (4, 0x060D6160), (8, 0), (12, 0x060D9999), (16, 0x060D61A0)]:
        data[offset:offset+4] = address.to_bytes(4, 'big')
    assert make_config.header_seeds(data, 0x060D6000) == {0x060D6140, 0x060D6160, 0x060D61A0}


def sub_menu_fixture():
    base = 0x060D6000
    data = bytearray(0x100)
    for index, offset in enumerate(make_config.SUB_MENU_FUNCTION_FIELDS):
        callback = base + 0x60 + index * 4
        data[offset:offset + 4] = callback.to_bytes(4, 'big')
        data[callback - base:callback - base + 4] = RTS_NOP
    # Campo sem consumidor de função comprovado não ganha tipo pelo valor.
    for offset in (0x0C,):
        data[offset:offset + 4] = (base + 0xF0).to_bytes(4, 'big')
    return base, data


def test_sub_menu_keeps_typed_indirect_exports_after_header_is_sliced(tmp_path):
    from saturnrecomp import sh2
    from saturnrecomp.recomp import discover

    base, data = sub_menu_fixture()
    (tmp_path / 'SUB.PRG').write_bytes(data)
    registered = [module for module in make_config.MODULES if module[0] == 'SUB']
    modules = make_config.module_entries(tmp_path, tmp_path, registered, slices=tmp_path / 'slices')
    module = modules[0]
    # O fatal real veio do campo +0x3C, ausente na tabela gerada antiga.
    fatal_callback = int.from_bytes(data[0x3C:0x40], 'big')
    image = sh2.Image(Path(module['file']).read_bytes(), module['base'])
    generated = discover.Program(image, module['seeds'])
    assert fatal_callback in generated.funcs
    assert set(module['seeds']) == make_config.sub_menu_seeds(data, base)
    assert len(module['seeds']) == 18
    assert base + 0xF0 not in generated.funcs
    assert module['source_offset'] == 0x60
    assert Path(module['file']).read_bytes() == data[0x60:0xA8]
    # Somente a primeira entrada não alcança os callbacks invocados pelo GAME.
    old = discover.Program(image, [make_config.entry_seed(data, base)])
    assert fatal_callback not in old.funcs


@pytest.mark.parametrize('value', [0, 0x060D6004, 0x060D6061, 0x060D6100])
def test_sub_menu_rejects_invalid_function_field(value):
    base, data = sub_menu_fixture()
    data[0x3C:0x40] = value.to_bytes(4, 'big')
    with pytest.raises(SystemExit, match=r'SUB: callback inválido no campo \+3C'):
        make_config.sub_menu_seeds(data, base)


def test_sub_menu_requires_all_nineteen_header_fields():
    with pytest.raises(SystemExit, match='19 campos truncado'):
        make_config.sub_menu_seeds(bytes(0x4B), 0x060D6000)


def test_alucard_second_weapon_record_keeps_callbacks_and_disc_offset(tmp_path):
    # A partida gravada carrega o registro 52 na segunda regiao de armas.
    registered = [module for module in make_config.MODULES if module[0] == 'WEAPON1_52']
    assert registered == [('WEAPON1_52', 'WEAPON1_52.PRG', 0x060D9000, None)]
    base = 0x060D9000
    record = bytearray(0x3000)
    record[:4] = (base + 0x48).to_bytes(4, 'big')
    record[4:8] = (base + 0x50).to_bytes(4, 'big')
    # zero_3.c usa o campo +0x40 como recurso de sprites, nao como callback.
    record[0x40:0x44] = (base + 0x60).to_bytes(4, 'big')
    record[0x48:0x4C] = RTS_NOP
    record[0x50:0x54] = RTS_NOP
    (tmp_path / 'WEAPON1.PRG').write_bytes(bytes(52 * 0x3000) + record + bytes(0x3000))
    modules = make_config.module_entries(tmp_path, tmp_path, registered, slices=tmp_path / 'slices')
    assert modules[0]['seeds'] == [base + 0x48, base + 0x50]
    assert modules[0]['source'] == '/WEAPON1.PRG'
    assert modules[0]['source_offset'] == 52 * 0x3000 + 0x48
    assert modules[0]['base'] == base + 0x48
    assert Path(modules[0]['file']).read_bytes() == record[0x48:0x54]


def test_short_sword_record_exports_weapon_id_callback_and_disc_offset(tmp_path):
    # ALUCARD chama o campo +0x18 da arma; o registro 37 atende a short sword.
    registered = [module for module in make_config.MODULES if module[0] == 'WEAPON0_37']
    assert registered == [('WEAPON0_37', 'WEAPON0_37.PRG', 0x060D6000, None)]
    assert make_config.CODE_RANGES['WEAPON0_37'] == (0x60, 0xDF4)
    base = 0x060D6000
    record = bytearray(0x3000)
    record[:4] = (base + 0x128).to_bytes(4, 'big')
    record[0x18:0x1C] = (base + 0xD1C).to_bytes(4, 'big')
    record[0x40:0x44] = (base + 0xF00).to_bytes(4, 'big')
    record[0x60:0x64] = RTS_NOP
    record[0x128:0x12C] = RTS_NOP
    record[0xD1C:0xD20] = RTS_NOP
    (tmp_path / 'WEAPON0.PRG').write_bytes(bytes(37 * 0x3000) + record + bytes(0x3000))
    modules = make_config.module_entries(tmp_path, tmp_path, registered, slices=tmp_path / 'slices')
    assert modules[0]['seeds'] == [base + 0x60, base + 0x128, 0x060D6D1C]
    assert base + 0xF00 not in modules[0]['seeds']
    assert modules[0]['source'] == '/WEAPON0.PRG'
    assert modules[0]['source_offset'] == 37 * 0x3000 + 0x60
    assert modules[0]['base'] == base + 0x60
    assert Path(modules[0]['file']).read_bytes() == record[0x60:0xDF4]


def test_known_code_slice_is_unchanged_when_trailing_palette_mutates(tmp_path, monkeypatch):
    base = 0x060D6000
    data = bytearray(0x200)
    data[:4] = (base + 0x60).to_bytes(4, 'big')
    data[0x60:0x64] = RTS_NOP
    program = tmp_path / 'RANGED.PRG'
    program.write_bytes(data)
    monkeypatch.setattr(make_config, 'CODE_RANGES', {'RANGED': (0x60, 0x80)})
    modules = [('RANGED', 'RANGED.PRG', base, None)]
    before = make_config.module_entries(tmp_path, tmp_path, modules, slices=tmp_path / 'before')
    before_bytes = Path(before[0]['file']).read_bytes()
    data[0x90] |= 0x80
    program.write_bytes(data)
    after = make_config.module_entries(tmp_path, tmp_path, modules, slices=tmp_path / 'after')
    assert Path(after[0]['file']).read_bytes() == before_bytes == data[0x60:0x80]
    assert before[0]['source_offset'] == after[0]['source_offset'] == 0x60
    assert before[0]['seeds'] == after[0]['seeds'] == [base + 0x60]


def test_castle_entrance_has_own_module_and_observed_callback():
    matches = [m for m in make_config.MODULES if m[0] == 'STAGE_0X']
    assert matches == [('STAGE_0X', 'STAGE_0X.PRG', 0x060DC000, None)]
    assert {0x060E263C, 0x060DDAA4} <= make_config.OBSERVED_ENTRIES['STAGE_0X']
    assert 0x060F24D0 not in make_config.OBSERVED_ENTRIES['STAGE_0X']
    assert 'SUB' not in make_config.OBSERVED_ENTRIES

def test_castle_entity_72_callback_belongs_to_stage_code_not_sprite_bank():
    # Header -> EntityEntry** 060F3E60, slot71 -> 060FA134.
    # EntityEntry {spriteBank=060FA128, func=060EFFD4}; fatal PR0606C38C.
    callback = 0x060EFFD4
    assert callback in make_config.OBSERVED_ENTRIES['STAGE_0X']
    assert 0x060FA128 not in make_config.OBSERVED_ENTRIES['STAGE_0X']
    start, end = make_config.CODE_RANGES['STAGE_0X']
    assert 0x060DC000 + start <= callback < 0x060DC000 + end
    assert all(callback not in entries for name, entries in make_config.OBSERVED_ENTRIES.items()
               if name != 'STAGE_0X')


def entity_stage_fixture():
    base = 0x060DC000
    data = bytearray(0x200)
    for offset, value in [(0, base + 0x100), (0x100, base + 0x120),
                          (0x104, 0x06044348), (0x108, base + 0x128),
                          (0x120, base + 0x140), (0x124, base + 0x40),
                          (0x128, base + 0x150), (0x12C, base + 0x60)]:
        data[offset:offset + 4] = value.to_bytes(4, 'big')
    data[0x40:0x44] = RTS_NOP
    data[0x60:0x64] = RTS_NOP
    return base, data


def test_stage_entity_seeds_follow_records_and_ignore_banks_and_shared_noop():
    base, data = entity_stage_fixture()
    assert make_config.stage_entity_seeds(data, base) == {base + 0x40, base + 0x60}


@pytest.mark.parametrize(('offset', 'value'), [
    (0, 0x060DFFFF), (0x100, 0x060DC1FC),
    (0x100, 0x06044000), (0x124, 0x060DC041), (0x124, 0),
])
def test_stage_entity_seeds_reject_invalid_tables_and_callbacks(offset, value):
    base, data = entity_stage_fixture()
    data[offset:offset + 4] = value.to_bytes(4, 'big')
    with pytest.raises(SystemExit):
        make_config.stage_entity_seeds(data, base)


def test_stage_entity_seeds_require_null_terminator():
    base, data = entity_stage_fixture()
    data[0:4] = (base + 0x1FC).to_bytes(4, 'big')
    data[0x1FC:0x200] = (base + 0x120).to_bytes(4, 'big')
    with pytest.raises(SystemExit):
        make_config.stage_entity_seeds(data, base)


@pytest.mark.parametrize(('name', 'filename', 'expected_end'), [
    ('STAGE_00', 'STAGE_00.PRG', 0x64),
    ('STAGE_0X', 'STAGE_0X.PRG', 0x80),
    ('STAGE_02', 'STAGE_02.PRG', 0x80),
])
def test_stage_mapping_uses_typed_callbacks_instead_of_header_tables(
        tmp_path, monkeypatch, name, filename, expected_end):
    registered = [module for module in make_config.MODULES if module[0] == name]
    assert registered == [(name, filename, 0x060DC000, None)]
    assert filename in make_config.TYPED_STAGE_PROGRAMS
    if name == 'STAGE_02':
        assert make_config.CODE_RANGES[name] == (0x40, 0x10C50)
    base, data = entity_stage_fixture()
    (tmp_path / filename).write_bytes(data)
    # As faixas sinteticas exercitam o recorte sem carregar bytes do jogo.
    monkeypatch.setattr(make_config, 'CODE_RANGES', {
        'STAGE_0X': (0x40, 0x80), 'STAGE_02': (0x40, 0x80),
    })
    monkeypatch.setattr(make_config, 'OBSERVED_ENTRIES', {'STAGE_0X': {base + 0x40}})
    modules = make_config.module_entries(tmp_path, tmp_path,
        [(name, filename, base, None)], slices=tmp_path / 'slices')
    assert modules[0]['seeds'] == [base + 0x40, base + 0x60]
    assert base + 0x100 not in modules[0]['seeds']
    assert base + 0x140 not in modules[0]['seeds']
    assert base + 0x150 not in modules[0]['seeds']
    assert Path(modules[0]['file']).read_bytes() == data[0x40:expected_end]
    assert modules[0]['base'] == base + 0x40
    assert modules[0]['source'] == '/' + filename
    assert modules[0]['source_offset'] == 0x40
    assert not any(name == 'STAGE_0X_HIGH' for name, *_ in make_config.MODULES)


@pytest.mark.parametrize(('name', 'filename'), [
    ('STAGE_0X', 'STAGE_0X.PRG'), ('STAGE_02', 'STAGE_02.PRG'),
])
def test_stage_mapping_rejects_exported_callback_outside_known_code(
        tmp_path, monkeypatch, name, filename):
    assert filename in make_config.TYPED_STAGE_PROGRAMS
    base, data = entity_stage_fixture()
    data[0x12C:0x130] = (base + 0x90).to_bytes(4, 'big')
    (tmp_path / filename).write_bytes(data)
    monkeypatch.setattr(make_config, 'CODE_RANGES', {name: (0x40, 0x80)})
    monkeypatch.setattr(make_config, 'OBSERVED_ENTRIES', {})
    with pytest.raises(SystemExit, match='fora do recorte'):
        make_config.module_entries(tmp_path, tmp_path,
            [(name, filename, base, None)], slices=tmp_path / 'slices')


def test_code_ranges_exclude_mutable_middle_and_keep_source_offsets(tmp_path, monkeypatch):
    extract = tmp_path / 'extract'
    extract.mkdir()
    base = 0x060DC000
    data = bytearray(range(256))
    data[:4] = (base + 0x20).to_bytes(4, 'big')
    (extract / 'STAGE.PRG').write_bytes(data)
    monkeypatch.setattr(make_config, 'CODE_RANGES', {'LOW': (0x20, 0x40), 'HIGH': (0x80, 0xA0)})
    monkeypatch.setattr(make_config, 'OBSERVED_ENTRIES', {'HIGH': {base + 0x90}})
    mods = make_config.module_entries(extract, tmp_path, [
        ('LOW', 'STAGE.PRG', base, None), ('HIGH', 'STAGE.PRG', base, None)], slices=tmp_path / 'slices')
    assert [m['source_offset'] for m in mods] == [0x20, 0x80]
    assert [m['base'] for m in mods] == [base + 0x20, base + 0x80]
    assert mods[1]['seeds'] == [base + 0x80, base + 0x90]
    assert Path(mods[0]['file']).read_bytes() == data[0x20:0x40]
    assert Path(mods[1]['file']).read_bytes() == data[0x80:0xA0]
