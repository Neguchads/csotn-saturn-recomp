"""Relata recortes e mapas estáticos, sem inferir progresso ou execução do jogo."""
import argparse
import hashlib
import json
from pathlib import Path

from saturnrecomp import build, config, inventory

DEFAULT_CONFIG = Path(__file__).resolve().parents[1] / 'game-jp-all.toml'


def union_ranges(ranges):
    result = []
    for start, end in sorted(ranges):
        if result and start <= result[-1][1]:
            result[-1][1] = max(result[-1][1], end)
        else:
            result.append([start, end])
    return result


def complement(ranges, size):
    result, at = [], 0
    for start, end in ranges:
        if start > at:
            result.append([at, start])
        at = end
    if at < size:
        result.append([at, size])
    return result


def create_report(config_path):
    game = config.load(config_path)
    build_dir = Path(game.build)
    manifest_path = Path(game.inventory.manifest) if game.inventory else build_dir / 'analysis/disc.json'
    extract = Path(game.inventory.files) if game.inventory else build_dir / 'extract'
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    sources = manifest['files']
    intervals, mapped_ranges, cached_sources = {}, {}, {}
    modules, issues = [], []
    for name, _, _, seeds in build.specs(game):
        module = game.module(name)
        item = {'module': name, 'source': module.source, 'source_offset': module.source_offset}
        modules.append(item)
        try:
            data = Path(module.file).read_bytes()
            item['bytes'] = len(data)
            item['sha1'] = hashlib.sha1(data).hexdigest()
            if not module.source or module.source not in sources:
                raise ValueError('explicit source missing from manifest')
            if module.source not in cached_sources:
                original = (extract / module.source.lstrip('/')).read_bytes()
                metadata = sources[module.source]
                if len(original) != metadata['size'] or hashlib.sha1(original).hexdigest() != metadata['sha1']:
                    raise ValueError('source differs from manifest')
                cached_sources[module.source] = original
            original = cached_sources[module.source]
            start, end = module.source_offset, module.source_offset + len(data)
            if start < 0 or not data or end > len(original) or original[start:end] != data:
                raise ValueError('source bytes differ from prepared slice or range is invalid')
            intervals.setdefault(module.source, []).append((start, end))
            map_path = build_dir / 'recomp/map' / f'{name}.tsv'
            if not map_path.is_file():
                raise ValueError('map missing; generate first')
            map_sha1, mapped = inventory.read_map(map_path)
            if map_sha1 != item['sha1']:
                raise ValueError('map SHA-1 differs from prepared slice; regenerate first')
            addresses = {address for address, entries in mapped.items()
                         if any(entry[1] in ('insn', 'slot') for entry in entries)}
            if any(address < module.base or address + 2 > module.base + len(data) for address in addresses):
                raise ValueError('map contains an instruction outside the prepared slice')
            mapped_ranges.setdefault(module.source, []).extend(
                (start + address - module.base, start + address - module.base + 2) for address in addresses)
            analysis, gaps, problems = inventory.image(module, seeds, str(extract), {}, str(build_dir / 'recomp/map'))
            item['analysis'] = analysis
            if analysis['omitted'] and not problems:
                issues.append(f'{name}: map omits {analysis["omitted"]} discovered instructions')
            item['gaps'] = [{'address': address, 'slice_offset': offset, 'bytes': size,
                             'content': content, 'prologues': prologues, 'pointers_in': pointers,
                             'decodes': decodes}
                            for _, address, offset, size, content, prologues, pointers, decodes in gaps]
            issues.extend(problems)
        except (OSError, ValueError, KeyError) as error:
            issues.append(f'{name}: {error}')
    files = []
    for source, metadata in sorted(sources.items()):
        included = union_ranges(intervals.get(source, []))
        mapped = union_ranges(mapped_ranges.get(source, []))
        unknown = complement(included, metadata['size'])
        files.append({'source': source, 'bytes': metadata['size'], 'sha1': metadata['sha1'],
                      'included_ranges': included, 'included_bytes': sum(end - start for start, end in included),
                      'mapped_instruction_bytes': sum(end - start for start, end in mapped),
                      'unknown_ranges': unknown, 'unknown_bytes': sum(end - start for start, end in unknown)})
    return {
        'schema_version': 1, 'kind': 'static-inventory',
        'status': 'invalid' if issues else 'valid', 'runtime_validation': 'not_measured',
        'config': str(Path(config_path).resolve()), 'manifest': str(manifest_path.resolve()),
        'manifest_sha1': hashlib.sha1(manifest_path.read_bytes()).hexdigest(),
        'scope': 'Manifest files only; excludes system area and audio tracks. Ranges are half-open byte offsets.',
        'interpretation': 'Included bytes are prepared slices, not all instructions. Unknown bytes are outside '
                          'those slices, not proven code or data. Mapped instructions do not prove execution.',
        'files': files, 'modules': modules, 'issues': issues,
        'totals': {key: sum(row[key] for row in files)
                   for key in ('bytes', 'included_bytes', 'mapped_instruction_bytes', 'unknown_bytes')},
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('config', nargs='?', type=Path, default=DEFAULT_CONFIG)
    parser.add_argument('--out', type=Path)
    args = parser.parse_args(argv)
    try:
        report = create_report(args.config)
        output = args.out or Path(config.load(args.config).build) / 'inventory-jp.json'
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    except (OSError, ValueError, KeyError) as error:
        parser.exit(1, f'Inventory failed: {error}\n')
    print(f"{report['status']}: {output}; runtime_validation=not_measured")
    for issue in report['issues']:
        print(issue)
    return 1 if report['issues'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
