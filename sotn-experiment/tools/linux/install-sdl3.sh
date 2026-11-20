#!/bin/bash
# Ubuntu 24.04 não tem libsdl3-dev no apt. Compila o SDL3 da tag oficial, na versão da baseline
# do pc-saturnbomberman (docs/BASELINE.md), e instala em ~/.local/sdl3 sem tocar no sistema.
set -euo pipefail
TAG=release-3.4.14
SRC="$HOME/work/SDL-$TAG"
PREFIX="$HOME/.local/sdl3"
[ -d "$SRC" ] || git clone -q --depth 1 --branch "$TAG" https://github.com/libsdl-org/SDL.git "$SRC"
cmake -S "$SRC" -B "$SRC/build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF > "$SRC/configure.log"
ninja -C "$SRC/build" install > "$SRC/build.log"
ls "$PREFIX/lib/cmake/SDL3/SDL3Config.cmake"
