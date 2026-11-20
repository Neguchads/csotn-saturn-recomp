"""Não promover dados com desvio em delay slot a funções especulativas."""
import struct

import pytest

from saturnrecomp import sh2
from saturnrecomp.recomp import discover, emit


BASE = 0x1000
FAKE = BASE + 0x10
REAL = BASE + 0x30
CALLEE = BASE + 0x40


def pointer_pool_image(slot):
    data = bytearray(b'\xff' * 0x50)

    def words(offset, *values):
        struct.pack_into('>' + 'H' * len(values), data, offset, *values)

    words(0, 0x000B, 0x0009)  # entrada real: rts; nop
    struct.pack_into('>II', data, 4, FAKE, REAL)  # pool de ponteiros
    # Registro falso que parece código longo, mas contém controle no delay slot.
    words(0x10, 0xB016, slot, *([0xE000] * 6), 0x000B, 0x0009)
    words(0x30, 0x2F86, 0xE001, 0x68F6, 0x000B, 0x0009)
    words(0x40, 0x000B, 0x0009)
    return sh2.Image(bytes(data), BASE)


@pytest.mark.parametrize('slot', [0x8900, 0x8B00])
def test_speculative_branch_in_delay_slot_stays_data_and_real_code_emits(slot):
    program = discover.Program(pointer_pool_image(slot), [BASE])

    assert FAKE not in program.funcs
    assert FAKE not in program.code
    assert REAL in program.funcs
    assert program.funcs[REAL].bad is None
    for function in program.funcs.values():
        lines = emit.Body(program, function, set(program.funcs)).emit()
        assert lines[0].startswith('void f_')


@pytest.mark.parametrize('slot', [0x8900, 0x8B00])
def test_explicit_entry_with_illegal_delay_slot_is_kept_as_diagnostic(slot):
    program = discover.Program(pointer_pool_image(slot), [BASE, FAKE])

    assert FAKE in program.funcs
    assert program.funcs[FAKE].bad == ('bad slot', FAKE + 2)
    with pytest.raises(emit.Unsupported):
        emit.Body(program, program.funcs[FAKE], set(program.funcs)).emit()


def test_legal_delayed_call_keeps_real_pointer_function_and_its_callee():
    program = discover.Program(pointer_pool_image(0x0009), [BASE])

    assert {BASE, FAKE, REAL, CALLEE} <= set(program.funcs)
    assert program.funcs[FAKE].bad is None
    assert CALLEE in program.funcs[FAKE].calls
    for function in program.funcs.values():
        emit.Body(program, function, set(program.funcs)).emit()
