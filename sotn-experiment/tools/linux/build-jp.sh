#!/bin/bash
# Compila o C++ gerado por tools/analyze-jp.py (build/jp/recomp) com o runtime do saturn-recomp.
# Sem SDL3 instalado o CMake monta o host headless (sem janela, som ou controle).
set -euo pipefail
EXP="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
RUNTIME="${SATURN_RUNTIME:-$EXP/../engine/runtime}"
OUT="$HOME/work/sotn-jp-build"
[ -d "$RUNTIME" ] || { echo "rode check-engine.sh antes"; exit 1; }
cmake -S "$EXP/build/jp/recomp" -B "$OUT" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=clang++ -DSATURN_RUNTIME="$RUNTIME" ${SATURN_CMAKE_EXTRA:-}
ninja -C "$OUT" -j "${JOBS:-4}" saturn selftest
ls -la "$OUT/saturn" "$OUT/selftest"
