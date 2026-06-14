#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${TEXRENDER_BUILD_DIR:-$ROOT_DIR/build}"
SIGNAL_EXE="${TEXRENDER_SIGNAL_EXE:-$BUILD_DIR/texrender_realtime_values}"
OUTPUT="${1:-$ROOT_DIR/docs/realtime-signal.gif}"
WORK_DIR="${TMPDIR:-/tmp}/texrender-realtime-signal-demo"
FPS="${TEXRENDER_SIGNAL_GIF_FPS:-12}"
FRAMES="${TEXRENDER_SIGNAL_GIF_FRAMES:-48}"
START="${TEXRENDER_SIGNAL_GIF_START:-2.0}"
STEP="${TEXRENDER_SIGNAL_GIF_STEP:-0.18}"

if [[ ! -x "$SIGNAL_EXE" ]]; then
    printf 'missing realtime signal executable: %s\n' "$SIGNAL_EXE" >&2
    printf 'build the raylib backend first, for example: cmake -S . -B build && cmake --build build --target texrender_realtime_values -j\n' >&2
    exit 1
fi

if ! command -v ffmpeg >/dev/null 2>&1; then
    printf 'ffmpeg is required to generate %s\n' "$OUTPUT" >&2
    exit 1
fi

rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR/frames"

for ((i = 0; i < FRAMES; ++i)); do
    frame="$(printf '%s/frames/frame%03d.png' "$WORK_DIR" "$((i + 1))")"
    clock="$(awk -v start="$START" -v step="$STEP" -v i="$i" 'BEGIN { printf "%.4f", start + step * i }')"
    TEXRENDER_SIGNAL_SCREENSHOT="$frame" TEXRENDER_SIGNAL_CLOCK="$clock" "$SIGNAL_EXE" >/dev/null
done

ffmpeg -v warning -y -framerate "$FPS" -i "$WORK_DIR/frames/frame%03d.png" \
    -vf "scale=920:-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse=dither=bayer:bayer_scale=4" \
    -loop 0 "$OUTPUT"

printf 'realtime signal GIF written to %s\n' "$OUTPUT"
