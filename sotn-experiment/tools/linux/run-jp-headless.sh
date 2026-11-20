#!/bin/bash
# Execução headless do build do learn (build/jp/recomp-build/saturn): roda N VBlanks e tira capturas.
# Uso: run-jp-headless.sh [VBLANKS] [SHOTS] [INPUT]   ex.: run-jp-headless.sh 3600 600,1500,3600
# INPUT é o --input do runtime (VBLANK:BOTOES,...), ex.: 2000:START,2010:
set -uo pipefail
EXP="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
ROMDIR="${SOTN_ROM_DIR:-$EXP/../roms/akumajou-dracula-x-gekka-no-yasoukyoku-japan-chd-iso-1998}"
BIN="$EXP/build/jp/recomp-build/saturn"
OUT="$EXP/build/jp/run"
export LD_LIBRARY_PATH="$HOME/.local/sdl3/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
VBLANKS="${1:-600}"
SHOTS="${2:-120,300,600}"
INPUT=()
[ -n "${3:-}" ] && INPUT=(--input "$3")
cue=("$ROMDIR"/*.cue)
[ "${#cue[@]}" -eq 1 ] && [ -f "${cue[0]}" ] || { echo "esperado um CUE em $ROMDIR"; exit 1; }
OUT="$OUT/$(date +%Y%m%d-%H%M%S)-$$"; mkdir -p "$OUT"
timeout "${TIMEOUT:-900}" "$BIN" --cue "${cue[0]}" --headless --save - --out "$OUT" --vblanks "$VBLANKS" \
    --shot "$SHOTS" "${INPUT[@]}" > "$OUT/run.log" 2>&1
code=$?
echo "saida: $code"
grep -vE "^\s+r[0-9]" "$OUT/run.log" | tail -n 12
ls "$OUT"
exit $code
