import importlib.util
import json
from pathlib import Path

import pytest


def load_tool():
    path = Path(__file__).resolve().parents[1] / 'tools' / 'generate-jp.py'
    spec = importlib.util.spec_from_file_location('generate_jp', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def write_config(root, options=''):
    (root / 'program.bin').write_bytes(bytes.fromhex('000B000900090009000B0009'))
    (root / 'learned.json').write_text(json.dumps({'TEST': ['06004088']}))
    path = root / 'game.toml'
    path.write_text('''[game]
name = "Synthetic SH2"
build = "out"
seeds = "learned.json"
[[module]]
name = "TEST"
file = "program.bin"
base = 0x06004080
seeds = [0x06004080]
callable_entry = true
''' + options)
    return path


def test_generation_emits_declared_and_learned_functions_without_building(tmp_path, monkeypatch):
    tool = load_tool()
    config = write_config(tmp_path)
    def forbidden(*args, **kwargs):
        pytest.fail('Geracao Windows nao deve executar prereqs, CMake ou Ninja.')
    monkeypatch.setattr(tool.build, 'recompile', forbidden)
    monkeypatch.setattr(tool.build.prereqs, 'check', forbidden)
    monkeypatch.setattr(tool.build, '_run', forbidden)
    tool.generate_config(config, log=lambda message: None)
    output = tmp_path / 'out' / 'recomp'
    table = (output / 'p_test_table.cpp').read_text()
    assert '06004080' in table
    assert '06004088' in table
    assert (output / 'CMakeLists.txt').is_file()
    record = json.loads((output / 'generation-inputs.json').read_text())
    assert record['inputs'] == tool.build.inputs(tool.config.load(config))
    assert record['kind'] == 'generation-only'
    assert not (output / 'inputs.json').exists()
    assert not (tmp_path / 'out' / 'recomp-build').exists()


def test_generation_forwards_runtime_options_and_task_switches(tmp_path, monkeypatch):
    tool = load_tool()
    path = write_config(tmp_path, '''runtime_literals = true
unwind_returns = true
[tasks]
module = "TEST"
setjmp = 0x06004080
longjmp = 0x06004088
yields = [0x06004080]
[[task_switch]]
module = "TEST"
save = 0x06004080
save_key = "r1"
resume_sp = 84
switch = 0x06004088
switch_key = "r0"
[[hook]]
module = "TEST"
at = 0x06004080
''')
    captured = {}
    specs = [('TEST', 'synthetic', 0x06004080, [0x06004080, 0x06004088])]
    monkeypatch.setattr(tool.build, 'specs', lambda game, log: specs)
    def capture(actual_specs, out, **kwargs):
        captured.update(kwargs)
        assert actual_specs is specs
        Path(out).mkdir(parents=True)
    monkeypatch.setattr(tool, 'generate', capture)
    tool.generate_config(path, log=lambda message: None)
    assert captured['tasks'] == (0x06004080, 0x06004088)
    # O loader acrescenta os hooks de tasks e task_switch antes do hook explicito.
    assert captured['hooks'] == {'TEST': [0x06004080, 0x06004088,
                                        0x06004080, 0x06004088, 0x06004080]}
    assert captured['runtime_literals'] == {'TEST'}
    assert captured['callable_entries'] == {'TEST'}
    assert captured['unwind_returns'] == {'TEST'}
    assert captured['task_switches'] == [(0x06004080, 1, 84, 0x06004088, 0)]


def test_generation_rejects_missing_program_before_creating_output(tmp_path):
    tool = load_tool()
    path = write_config(tmp_path)
    (tmp_path / 'program.bin').unlink()
    with pytest.raises(SystemExit, match='ausentes'):
        tool.generate_config(path)
    assert not (tmp_path / 'out').exists()
