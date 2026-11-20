"""Gera C++ a partir do TOML sem executar CMake, Ninja ou compilar Linux."""
import argparse
import json
from pathlib import Path

from saturnrecomp import build, config
from saturnrecomp.recomp.__main__ import generate

DEFAULT_CONFIG = Path(__file__).resolve().parents[1] / 'game-jp-all.toml'


def generate_config(path, log=print):
    game = config.load(path)
    missing = [module.file for module in game.modules if not Path(module.file).is_file()]
    if missing:
        raise SystemExit('Programas ausentes; preparar o disco primeiro: ' + ', '.join(missing))
    inputs = build.inputs(game)
    output = Path(game.build) / 'recomp'
    generate(
        build.specs(game, log), str(output), log=log, hooks=game.hooks,
        tasks=(game.tasks.setjmp, game.tasks.longjmp) if game.tasks else None,
        runtime_literals={module.name for module in game.modules if module.runtime_literals},
        callable_entries={module.name for module in game.modules if module.callable_entry},
        unwind_returns={module.name for module in game.modules if module.unwind_returns},
        task_switches=[(item.save, item.save_key, item.resume_sp, item.switch, item.switch_key)
                       for item in game.task_switches],
    )
    # O stamp do build Linux pertence ao motor. Este registro prova somente a geracao.
    record = {'kind': 'generation-only', 'inputs': inputs}
    (output / 'generation-inputs.json').write_text(
        json.dumps(record, indent=2, sort_keys=True) + '\n', encoding='utf-8')


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('config', nargs='?', type=Path, default=DEFAULT_CONFIG)
    args = parser.parse_args(argv)
    generate_config(args.config)


if __name__ == '__main__':
    main()
