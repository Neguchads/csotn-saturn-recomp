#!/bin/bash
set -euo pipefail
EXP="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
ROMDIR="${SOTN_ROM_DIR:-$EXP/../roms/akumajou-dracula-x-gekka-no-yasoukyoku-japan-chd-iso-1998}"
export LD_LIBRARY_PATH="$HOME/.local/sdl3/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "$EXP"
cue=("$ROMDIR"/*.cue)
[ "${#cue[@]}" -eq 1 ] && [ -f "${cue[0]}" ]
OUT=build/jp/validation-continue
mkdir -p "$OUT"
printf 'CUE=%s\n' "${cue[0]}"
timeout 180 build/jp/recomp-build/saturn --cue "${cue[0]}" --headless --save - --out "$OUT" --vblanks 14000 --shot 9000,10000,11000,12000,13500 --input '1600:START,1610:,1800:A,1810:,2300:A,2310:,2800:C,2810:,3300:START,3310:,3800:A,3810:,8900:LEFT,10000:,10010:C,10030:,10100:LEFT,10800:,10810:C,10830:,10900:LEFT,12000:' --wav "$OUT/audio.wav" > "$OUT/run.log" 2>&1
