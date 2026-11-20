import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import make_all_config


ADDRESS = 0x060DC000


@pytest.mark.parametrize('name', ['BOSS_07', 'BOSS_04', 'BOSS_R09'])
def test_secondary_boss_dispatch_reads_only_proven_slots_and_checks_consumer(name):
    consumer, table, slots = make_all_config.SECONDARY_DISPATCH[name]
    data = bytearray(0x30000)
    offset = consumer - ADDRESS
    data[offset:offset + 8] = bytes.fromhex('d13b011e410b6483')
    literal = ((consumer + 4) & ~3) + 0x3B * 4 - ADDRESS
    put_word(data, literal, table)
    for slot, callback in slots.items():
        put_word(data, table - ADDRESS + slot * 4, callback)
    # Um valor adjacente sem consumidor comprovado não vira seed.
    put_word(data, table - ADDRESS + 4, ADDRESS + 0x62)
    assert make_all_config.secondary_dispatch_seeds(name, data) == set(slots.values())
    data[offset + 4:offset + 6] = bytes.fromhex('400b')
    with pytest.raises(ValueError, match='consumidor'):
        make_all_config.secondary_dispatch_seeds(name, data)


def test_secondary_boss_dispatch_rejects_changed_callback_or_table():
    name = 'BOSS_07'
    consumer, table, slots = make_all_config.SECONDARY_DISPATCH[name]
    data = bytearray(0x30000)
    offset = consumer - ADDRESS
    data[offset:offset + 8] = bytes.fromhex('d13b011e410b6483')
    literal = ((consumer + 4) & ~3) + 0x3B * 4 - ADDRESS
    put_word(data, literal, table)
    for slot, callback in slots.items():
        put_word(data, table - ADDRESS + slot * 4, callback)
    put_word(data, table - ADDRESS + 9 * 4, ADDRESS + 0x64)
    with pytest.raises(ValueError, match='callback'):
        make_all_config.secondary_dispatch_seeds(name, data)
    put_word(data, literal, table + 4)
    with pytest.raises(ValueError, match='tabela'):
        make_all_config.secondary_dispatch_seeds(name, data)


def put_word(data, offset, value):
    data[offset:offset + 4] = value.to_bytes(4, 'big')


def stage_program():
    data = bytearray(0x140)
    put_word(data, 0x00, ADDRESS + 0x100)
    put_word(data, 0x04, ADDRESS + 0xB0)
    put_word(data, 0x100, ADDRESS + 0xD0)
    put_word(data, 0xD0, ADDRESS + 0xC0)
    put_word(data, 0xD4, ADDRESS + 0x60)
    data[0x60:0x64] = bytes.fromhex('000B0009')
    return data


def test_stage_follows_entity_callback_without_seeding_bank_or_header_data():
    data = stage_program()
    # O décimo campo pertence ao cabeçalho tipado; a palavra seguinte não.
    put_word(data, 9 * 4, ADDRESS + 0xA0)
    put_word(data, 10 * 4, ADDRESS + 0x50)
    seeds, boundary, shared = make_all_config.stage_layout(data, ADDRESS)
    assert seeds == {ADDRESS + 0x60}
    assert boundary == 0xA0
    assert shared == []


def test_stage_sprite_bank_delimits_data_without_becoming_a_callback():
    data = stage_program()
    put_word(data, 0x04, ADDRESS + 0xF0)
    seeds, boundary, _ = make_all_config.stage_layout(data, ADDRESS)
    assert seeds == {ADDRESS + 0x60}
    assert boundary == 0xC0


def test_stage_accepts_both_proven_shared_entity_entries_without_dereferencing():
    data = stage_program()
    put_word(data, 0x104, 0x06044348)
    put_word(data, 0x108, 0x060854D8)
    seeds, boundary, shared = make_all_config.stage_layout(data, ADDRESS)
    assert seeds == {ADDRESS + 0x60}
    assert boundary == 0xB0
    assert shared == [0x06044348, 0x060854D8]


def test_stage_rejects_unknown_shared_entry_instead_of_treating_it_as_code():
    data = stage_program()
    put_word(data, 0x104, 0x06044350)
    with pytest.raises(ValueError):
        make_all_config.stage_layout(data, ADDRESS)


@pytest.mark.parametrize('callback', [ADDRESS + 0x61, ADDRESS - 4, ADDRESS + 0x140])
def test_stage_rejects_odd_or_non_local_callbacks(callback):
    data = stage_program()
    put_word(data, 0xD4, callback)
    with pytest.raises(ValueError):
        make_all_config.stage_layout(data, ADDRESS)


@pytest.mark.parametrize('callback', [ADDRESS + 0xB0, ADDRESS + 0xB2])
def test_stage_rejects_callback_at_or_after_first_data_boundary(callback):
    data = stage_program()
    put_word(data, 0xD4, callback)
    with pytest.raises(ValueError):
        make_all_config.stage_layout(data, ADDRESS)


def test_stage_rejects_unterminated_entity_table():
    data = stage_program()
    for offset in range(0x100, len(data), 4):
        put_word(data, offset, ADDRESS + 0xD0)
    with pytest.raises(ValueError):
        make_all_config.stage_layout(data, ADDRESS)


def test_stage_rejects_table_with_only_shared_entries():
    data = stage_program()
    put_word(data, 0x100, 0x06044348)
    with pytest.raises(ValueError):
        make_all_config.stage_layout(data, ADDRESS)


@pytest.mark.parametrize('table', [ADDRESS + 0x101, ADDRESS - 4, ADDRESS + 0x140])
def test_stage_rejects_invalid_table_pointer(table):
    data = stage_program()
    put_word(data, 0x00, table)
    with pytest.raises(ValueError):
        make_all_config.stage_layout(data, ADDRESS)


def test_stage_rejects_truncated_header():
    with pytest.raises(ValueError):
        make_all_config.stage_layout(bytes(0x3F), ADDRESS)


def test_function_header_uses_sixteen_weapon_or_seventeen_player_fields():
    data = bytearray(0x100)
    put_word(data, 0x00, ADDRESS + 0x60)
    put_word(data, 0x3C, ADDRESS + 0x60)
    put_word(data, 0x40, ADDRESS + 0x80)
    put_word(data, 0x44, ADDRESS + 0xA0)
    assert make_all_config.function_header(data, ADDRESS, 16) == {ADDRESS + 0x60}
    assert make_all_config.function_header(data, ADDRESS, 17) == {
        ADDRESS + 0x60, ADDRESS + 0x80,
    }


def test_function_header_filters_header_addresses_odd_values_and_external_functions():
    data = bytearray(0x100)
    for offset, value in enumerate([
        ADDRESS + 0x20, ADDRESS + 0x61, ADDRESS + 0x100, 0x06044348, ADDRESS + 0x60,
    ]):
        put_word(data, offset * 4, value)
    assert make_all_config.function_header(data, ADDRESS, 16) == {ADDRESS + 0x60}


@pytest.mark.parametrize('count', [16, 17])
def test_function_header_rejects_truncated_typed_table(count):
    with pytest.raises(ValueError):
        make_all_config.function_header(bytes(count * 4 - 1), ADDRESS, count)


def test_function_header_rejects_program_without_local_callbacks():
    with pytest.raises(ValueError):
        make_all_config.function_header(bytes(0x100), ADDRESS, 16)


def test_make_slice_preserves_exact_bytes_offsets_and_sorted_seeds(tmp_path):
    data = bytes(range(0x80))
    slices = tmp_path / 'nested' / 'slices'
    result = make_all_config.make_slice(
        'WEAPON0_37', data, ADDRESS, {ADDRESS + 0x62, ADDRESS + 0x60},
        0x60, 0x68, '/WEAPON0.PRG', 37 * 0x3000, slices,
    )
    assert (slices / 'WEAPON0_37.bin').read_bytes() == data[0x60:0x68]
    assert result['base'] == ADDRESS + 0x60
    assert result['source'] == '/WEAPON0.PRG'
    assert result['source_offset'] == 37 * 0x3000 + 0x60
    assert result['seeds'] == [ADDRESS + 0x60, ADDRESS + 0x62]
    assert Path(result['file']) == slices / 'WEAPON0_37.bin'
    assert result['callable_entry'] is True


def test_stage16_identity_range_excludes_mutable_tail_and_keeps_last_function(tmp_path):
    source_end = make_all_config.STAGE_CODE_ENDS['STAGE_16']
    assert source_end == 0xCC10
    source = bytearray(0xD018)
    source[source_end:] = b'\xA5' * (len(source) - source_end)
    slices = tmp_path / 'stage16' / 'slices'
    seeds = {0x060DC040, 0x060E8AC0}

    result = make_all_config.make_slice(
        'STAGE_16', source, ADDRESS, seeds, 0x40, source_end,
        '/STAGE_16.PRG', 0, slices,
    )

    identity = (slices / 'STAGE_16.bin').read_bytes()
    assert len(identity) == 0xCBD0
    assert identity == bytes(source[0x40:source_end])
    assert max(seeds) < result['base'] + len(identity)


def test_stage15_identity_matches_crash_without_accepting_instruction_changes(tmp_path):
    import zlib
    end = make_all_config.STAGE_CODE_ENDS['STAGE_15']
    assert end == 0x14190
    source = bytearray(0x14B74)
    source[0x14150:0x14190] = bytes(range(64))
    source[0x14860:0x14864] = b'\xFF' * 4
    resident = bytearray(source)
    resident[0x14860:0x14864] = bytes.fromhex('01AE0125')
    seeds = {ADDRESS + 0x40, 0x060F00B8}
    slices = tmp_path / 'stage15'
    result = make_all_config.make_slice('STAGE_15', source, ADDRESS, seeds,
                                        0x40, end, '/STAGE_15.PRG', 0, slices)
    identity = (slices / 'STAGE_15.bin').read_bytes()
    assert len(identity) == 82256 and result['base'] == 0x060DC040
    assert identity == resident[0x40:end]
    assert max(seeds) < ADDRESS + end
    original = make_all_config.base.EXTRACT / 'STAGE_15.PRG'
    captured = make_all_config.base.ROOT.parent / 'logs-e-bugs/partidas/20261025-142750-774-22156/session-001/overlay-failure-fatal.bin'
    if original.is_file() and captured.is_file():
        source, resident = original.read_bytes(), captured.read_bytes()
        assert zlib.crc32(source[0x40:0x14B74]) != zlib.crc32(resident[0x40:0x14B74])
        assert source[0x40:end] == resident[0x40:end]
        assert zlib.crc32(resident[0x40:end]) == 0x3024B3DA
        corrupted = bytearray(resident[0x40:end]); corrupted[0] ^= 1
        assert zlib.crc32(corrupted) != zlib.crc32(source[0x40:end])


@pytest.mark.parametrize('seed', [ADDRESS + 0x5E, ADDRESS + 0x68, ADDRESS + 0x61])
def test_make_slice_rejects_seed_outside_code_or_odd_before_writing(tmp_path, seed):
    slices = tmp_path / 'slices'
    with pytest.raises(SystemExit):
        make_all_config.make_slice(
            'INVALID', bytes(0x80), ADDRESS, {seed}, 0x60, 0x68, '/TEST.PRG', 0, slices,
        )
    assert not (slices / 'INVALID.bin').exists()


@pytest.mark.parametrize('start,end', [(-1, 4), (4, 4), (8, 4), (0, 0x81)])
def test_make_slice_rejects_invalid_interval_before_writing(tmp_path, start, end):
    slices = tmp_path / 'slices'
    with pytest.raises(ValueError):
        make_all_config.make_slice(
            'INVALID', bytes(0x80), ADDRESS, set(), start, end, '/TEST.PRG', 0, slices,
        )
    assert not (slices / 'INVALID.bin').exists()
