#!/bin/bash
set -euo pipefail
EXP="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
ROMDIR="${SOTN_ROM_DIR:-$EXP/../roms/akumajou-dracula-x-gekka-no-yasoukyoku-japan-chd-iso-1998}"
export LD_LIBRARY_PATH="$HOME/.local/sdl3/lib"
cd "$EXP"
cue=("$ROMDIR"/*.cue)
OUT=build/jp/validation-faststart
mkdir -p "$OUT"
timeout 120 build/jp/recomp-build/saturn --cue "${cue[0]}" --headless --save - --out "$OUT" --vblanks 7000 --shot 4500,5000,5500,6500 --input '1600:START,1610:,1800:A,1810:,2300:A,2310:,2800:C,2810:,3300:START,3310:,3800:A,3810:,4200:START,4210:,4500:START,4510:,4700:LEFT,5900:' > "$OUT/run.log" 2>&1