#!/bin/bash
# Fluxo oficial do saturn-recomp para a edição japonesa, no Ubuntu/WSL:
#   testes do experimento, game-jp.toml, build (saturnrecomp.build) e o loop learn, que roda o jogo
#   headless, grava cada endereço que a descoberta perdeu em seeds-jp.json e recompila.
# Uso: learn-jp.sh [VBLANKS] [LIMITE] [INPUT]   INPUT = --input do runtime, ex.: 1600:START,1610:
set -euo pipefail
EXP="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
ROMDIR="${SOTN_ROM_DIR:-$EXP/../roms/akumajou-dracula-x-gekka-no-yasoukyoku-japan-chd-iso-1998}"
PY="$HOME/work/saturn-recomp/.venv/bin/python"
export CMAKE_PREFIX_PATH="$HOME/.local/sdl3"
export LD_LIBRARY_PATH="$HOME/.local/sdl3/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
VBLANKS="${1:-1800}"
LIMIT="${2:-40}"
INPUT=()
[ -n "${3:-}" ] && INPUT=(--input "$3")
cue=("$ROMDIR"/*.cue)
cd "$EXP"
"$PY" -m pytest -q tests
"$PY" tools/make_config.py
"$PY" -m saturnrecomp.build game-jp.toml
# As opções do learn vêm antes do toml: o argparse dele (nargs=REMAINDER) engole tudo depois.
# Sem --log: o learn lê a saída da execução para achar o endereço perdido (e a grava em learn/log.txt).
"$PY" -m saturnrecomp.learn --out build/jp/learn --limit "$LIMIT" game-jp.toml -- \
    --cue "${cue[0]}" --headless --save - --vblanks "$VBLANKS" "${INPUT[@]}"
