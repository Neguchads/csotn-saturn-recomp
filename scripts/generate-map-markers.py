"""Gera somente no PC a tabela de marcadores a partir de JSON fornecido pelo usuário."""
import argparse
import json
from pathlib import Path


def render(rows):
    if not isinstance(rows, list):
        raise ValueError('Esperada uma lista de marcadores')
    markers = []
    for row in rows:
        if not isinstance(row, dict) or set(row) != {'inverted', 'x', 'y', 'kind'}:
            raise ValueError('Cada marcador exige inverted, x, y e kind')
        if type(row['inverted']) is not bool:
            raise ValueError('inverted precisa ser booleano')
        if any(type(row[k]) is not int for k in ('x', 'y', 'kind')):
            raise ValueError('Coordenadas e tipo precisam ser inteiros')
        if not 0 <= row['x'] < 64 or not 0 <= row['y'] < 64 or row['kind'] not in (1, 2):
            raise ValueError('Coordenadas fora do mapa ou tipo inválido')
        markers.append(f"    {{{str(row['inverted']).lower()}, {row['x']}, {row['y']}, {row['kind']}}}")
    if not markers:
        return 'inline constexpr std::array<Marker, 0> kMarkers{};\n'
    return 'inline constexpr Marker kMarkers[] = {\n' + ',\n'.join(markers) + '\n};\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('json', type=Path)
    parser.add_argument('--out', type=Path, default=Path(__file__).resolve().parents[1] /
        'engine/runtime/src/host/sotn_map_markers.local.h')
    args = parser.parse_args()
    source = render(json.loads(args.json.read_text(encoding='utf-8-sig')))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text('// Dados locais; não publicar.\n' + source, encoding='utf-8')
    print(f'Tabela local gerada em {args.out}; este arquivo é ignorado pelo Git.')


if __name__ == '__main__':
    main()
