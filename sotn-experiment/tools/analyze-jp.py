"""Valida a edição japonesa e extrai os arquivos do disco. Não executa o jogo."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT.parent / 'referencias' / 'sotn-decomp'
BUILD = ROOT / 'build/jp'
ANALYSIS = BUILD / 'analysis'


def run(name, *args):
    with (ANALYSIS / name).open('w', encoding='utf-8') as log:
        subprocess.run([sys.executable, '-m', *map(str, args)], stdout=log,
                       stderr=subprocess.STDOUT, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cue', type=Path)
    args = parser.parse_args()
    if not args.cue.is_file() or args.cue.suffix.lower() != '.cue':
        raise SystemExit('Informe um CUE existente, com os BINs referenciados presentes.')
    ANALYSIS.mkdir(parents=True, exist_ok=True)
    run('disc-info.log', 'saturnrecomp.disc', args.cue, '--info')
    run('manifest.log', 'saturnrecomp.disc', args.cue, '--manifest', ANALYSIS / 'disc.json')
    run('extract.log', 'saturnrecomp.disc', args.cue, '--extract', BUILD / 'extract')
    rows = []
    for line in (REFERENCE / 'config/check.saturn.sha').read_text().splitlines():
        if not line.strip():
            continue
        expected, path = line.split()
        source = BUILD / 'extract' / Path(path).name
        actual = hashlib.sha1(source.read_bytes()).hexdigest() if source.exists() else None
        rows.append(dict(file=source.name, expected=expected, actual=actual, match=actual == expected))
    (ANALYSIS / 'retail-comparison.json').write_text(json.dumps(rows, indent=2))
    if len(rows) != 12 or not all(row['match'] for row in rows):
        raise SystemExit('Os 12 hashes de referência devem corresponder antes da geração.')
    # game-jp.toml agora vem de tools/make_config.py; a geração e o build, de saturnrecomp.build.
    print('12/12 hashes correspondem; extração pronta. Próximo: tools/make_config.py.')


if __name__ == '__main__':
    main()
