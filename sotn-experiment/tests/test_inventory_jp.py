import importlib.util
import json
from pathlib import Path

import pytest


TOOLS = Path(__file__).resolve().parents[1] / 'tools'


def load_tool(name):
    spec = importlib.util.spec_from_file_location(name.replace('-', '_'), TOOLS / f'{name}.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@pytest.fixture
def tool():
    return load_tool('inventory-jp')


def prepared_game(tmp_path, modules):
    """Cada tupla contém nome, arquivo fonte, offset e tamanho do recorte."""
    extract = tmp_path / 'build/jp/extract'
    extract.mkdir(parents=True)
    slices = tmp_path / 'slices'
    slices.mkdir()
    import hashlib
    sources = {source for _, source, _, _ in modules}
    manifest = {'files': {}}
    for source in sources:
        data = bytes.fromhex('000B0009') * 8
        (extract / source).write_bytes(data)
        manifest['files']['/' + source] = {
            'size': len(data), 'sha1': hashlib.sha1(data).hexdigest()}
    analysis = tmp_path / 'build/jp/analysis'
    analysis.mkdir()
    (analysis / 'disc.json').write_text(json.dumps(manifest))
    lines = ['[game]', 'build = "build/jp"']
    for name, source, offset, size in modules:
        data = (extract / source).read_bytes()[offset:offset + size]
        (slices / f'{name}.bin').write_bytes(data)
        lines.extend(['[[module]]', f'name = "{name}"', f'file = "slices/{name}.bin"',
                      'base = 0x06010000', f'source = "/{source}"', f'source_offset = {offset}'])
    config = tmp_path / 'game.toml'
    config.write_text('\n'.join(lines) + '\n')
    load_tool('generate-jp').generate_config(config, log=lambda _: None)
    return config


def test_union_counts_adjacent_and_overlapping_ranges_once(tool, tmp_path):
    config = prepared_game(tmp_path, [('A', 'PACK.PRG', 0, 8), ('B', 'PACK.PRG', 4, 8),
                                      ('C', 'PACK.PRG', 12, 4), ('D', 'PACK.PRG', 4, 4)])
    report = tool.create_report(config)
    assert report['status'] == 'valid'
    row = report['files'][0]
    assert row['included_ranges'] == [[0, 16]]
    assert row['included_bytes'] == 16
    assert row['mapped_instruction_bytes'] == 12
    assert row['unknown_ranges'] == [[16, 32]]
    assert row['unknown_bytes'] == 16
    assert report['runtime_validation'] == 'not_measured'


def test_same_guest_address_in_different_sources_counts_separately(tool, tmp_path):
    config = prepared_game(tmp_path, [('A', 'ONE.PRG', 0, 4), ('B', 'TWO.PRG', 0, 4)])
    report = tool.create_report(config)
    assert report['totals']['included_bytes'] == 8
    assert report['totals']['mapped_instruction_bytes'] == 8
    assert len(report['files']) == 2


def test_unmapped_source_remains_unknown(tool, tmp_path):
    config = prepared_game(tmp_path, [('A', 'PACK.PRG', 4, 4)])
    manifest = tmp_path / 'build/jp/analysis/disc.json'
    data = json.loads(manifest.read_text())
    data['files']['/UNUSED.PRG'] = {'size': 20, 'sha1': 'not-read'}
    manifest.write_text(json.dumps(data))
    report = tool.create_report(config)
    unused = next(row for row in report['files'] if row['source'] == '/UNUSED.PRG')
    assert unused['unknown_bytes'] == 20
    assert unused['included_ranges'] == []


@pytest.mark.parametrize('missing', [False, True])
def test_stale_or_missing_map_fails(tool, tmp_path, missing):
    config = prepared_game(tmp_path, [('A', 'PACK.PRG', 0, 4)])
    map_path = tmp_path / 'build/jp/recomp/map/A.tsv'
    if missing:
        # Uma fixture recém-gerada, sem qualquer dado do usuário.
        map_path.rename(map_path.with_suffix('.saved'))
    else:
        text = map_path.read_text()
        first, rest = text.split('\n', 1)
        map_path.write_text(first.split('sha1 ')[0] + 'sha1 ' + '0' * 40 + '\n' + rest)
    report = tool.create_report(config)
    assert report['status'] == 'invalid'
    assert any('map' in issue.lower() for issue in report['issues'])


def test_changed_slice_fails_and_cli_returns_error(tool, tmp_path):
    config = prepared_game(tmp_path, [('A', 'PACK.PRG', 0, 4)])
    (tmp_path / 'slices/A.bin').write_bytes(bytes.fromhex('7001000B'))
    report = tool.create_report(config)
    assert report['status'] == 'invalid'
    assert any('source bytes' in issue for issue in report['issues'])
    output = tmp_path / 'report.json'
    assert tool.main([str(config), '--out', str(output)]) == 1
    assert json.loads(output.read_text())['runtime_validation'] == 'not_measured'


def test_empty_map_body_fails_even_when_header_hash_matches(tool, tmp_path):
    config = prepared_game(tmp_path, [('A', 'PACK.PRG', 0, 4)])
    map_path = tmp_path / 'build/jp/recomp/map/A.tsv'
    lines = map_path.read_text().splitlines()
    map_path.write_text('\n'.join(line for line in lines if line.startswith('#')) + '\n')
    report = tool.create_report(config)
    assert report['status'] == 'invalid'
    assert any('map omits' in issue for issue in report['issues'])


def test_cli_defaults_to_configs_build_directory(tool, tmp_path, monkeypatch):
    config = prepared_game(tmp_path, [('A', 'PACK.PRG', 0, 4)])
    monkeypatch.chdir(tmp_path.parent)
    assert tool.DEFAULT_CONFIG.is_absolute()
    assert tool.main([str(config)]) == 0
    output = tmp_path / 'build/jp/inventory-jp.json'
    assert json.loads(output.read_text())['status'] == 'valid'
