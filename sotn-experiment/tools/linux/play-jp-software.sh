#!/bin/bash
set -euo pipefail
export SDL_VIDEO_DRIVER=x11
export SDL_RENDER_DRIVER=software
export SDL_AUDIO_DRIVER=pulseaudio
exec bash "$(dirname -- "${BASH_SOURCE[0]}")/play-jp.sh" "$@"