#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GALLERY_DIR="${GALLERY_DIR:-$ROOT_DIR/docs/gallery}"
OUTPUT="${1:-$ROOT_DIR/docs/live-preview.gif}"
WORK_DIR="${TMPDIR:-/tmp}/texgui-preview-gif"

if ! command -v ffmpeg >/dev/null 2>&1; then
    printf 'ffmpeg is required to generate %s\n' "$OUTPUT" >&2
    exit 1
fi

rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR/raw" "$WORK_DIR/normalized"

index=1
for slug in pythagorean fraction radical integral matrix colored; do
    input="$GALLERY_DIR/$slug.png"
    if [[ ! -f "$input" ]]; then
        printf 'missing gallery image: %s\n' "$input" >&2
        exit 1
    fi

    frame="$(printf 'frame%03d.png' "$index")"
    cp "$input" "$WORK_DIR/raw/$frame"
    ffmpeg -v warning -y -i "$WORK_DIR/raw/$frame" \
        -vf "scale=560:180:force_original_aspect_ratio=decrease:flags=lanczos,pad=640:240:(ow-iw)/2:(oh-ih)/2:color=white" \
        "$WORK_DIR/normalized/$frame" >/dev/null 2>&1
    index=$((index + 1))
done

ffmpeg -v warning -y -framerate 1 -i "$WORK_DIR/normalized/frame%03d.png" \
    -vf "split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse" \
    -loop 0 "$OUTPUT"

printf 'preview GIF written to %s\n' "$OUTPUT"
