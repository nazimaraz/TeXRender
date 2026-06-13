#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${TEXGUI_BUILD_DIR:-$ROOT_DIR/build}"
LIVE_EXE="${TEXGUI_LIVE_EXE:-$BUILD_DIR/texgui_live}"
OUTPUT="${1:-$ROOT_DIR/docs/live-preview.gif}"
WORK_DIR="${TMPDIR:-/tmp}/texgui-live-editor-demo"
FORMULA="${TEXGUI_LIVE_FORMULA:-\\int_{0}^{\\infty} e^{-x^2}\\,dx = \\frac{\\sqrt{\\pi}}{2}}"

if [[ ! -x "$LIVE_EXE" ]]; then
    printf 'missing live editor executable: %s\n' "$LIVE_EXE" >&2
    printf 'build the raylib backend first, for example: cmake -S . -B build && cmake --build build --target texgui_live -j\n' >&2
    exit 1
fi

if ! command -v ffmpeg >/dev/null 2>&1; then
    printf 'ffmpeg is required to generate %s\n' "$OUTPUT" >&2
    exit 1
fi

rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR/frames"

prefixes=(
    "\\int"
    "\\int_{0}^{\\infty}"
    "\\int_{0}^{\\infty} e^{-x^2}"
    "\\int_{0}^{\\infty} e^{-x^2}\\,dx"
    "$FORMULA"
)

index=1
for source in "${prefixes[@]}"; do
    frame="$(printf '%s/frames/frame%03d.png' "$WORK_DIR" "$index")"
    TEXGUI_SCREENSHOT="$frame" TEXGUI_LIVE_SOURCE="$source" TEXGUI_LIVE_TEXT_SIZE=56 "$LIVE_EXE" >/dev/null
    index=$((index + 1))
done

ffmpeg -v warning -y -framerate 1 -i "$WORK_DIR/frames/frame%03d.png" \
    -vf "scale=820:-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse" \
    -loop 0 "$OUTPUT"

printf 'live editor demo GIF written to %s\n' "$OUTPUT"
