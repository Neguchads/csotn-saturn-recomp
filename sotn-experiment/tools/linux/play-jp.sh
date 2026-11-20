#!/bin/bash
set -euo pipefail
EXP="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
ROMDIR="${SOTN_ROM_DIR:-$EXP/../roms/akumajou-dracula-x-gekka-no-yasoukyoku-japan-chd-iso-1998}"
export LD_LIBRARY_PATH="$HOME/.local/sdl3/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "$EXP"
cue=("$ROMDIR"/*.cue)
[ "${#cue[@]}" -eq 1 ] && [ -f "${cue[0]}" ] || { echo 'CUE não encontrado'; exit 1; }
[ -x build/jp/recomp-build/saturn ] || { echo 'Binário ausente; compilar antes.'; exit 1; }
OUT="build/jp/play/$(date +%Y%m%d-%H%M%S)"
mkdir -p "$OUT"
exec build/jp/recomp-build/saturn --cue "${cue[0]}" --out "$OUT" --scale 3 --record-input "$OUT/input.txt" "$@" > "$OUT/run.log" 2>&1