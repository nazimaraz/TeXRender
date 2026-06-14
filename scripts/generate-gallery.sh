#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FORMULAS_FILE="${1:-$ROOT_DIR/docs/gallery-formulas.txt}"
OUTPUT_DIR="${2:-$ROOT_DIR/build/gallery}"
BUILD_DIR="${TEXRENDER_BUILD_DIR:-$ROOT_DIR/build}"
OFFSCREEN_EXE="${TEXRENDER_OFFSCREEN_EXE:-$BUILD_DIR/texrender_offscreen}"
SVG_EXE="${TEXRENDER_SVG_EXPORT_EXE:-$BUILD_DIR/texrender_svg_export}"

if [[ ! -x "$OFFSCREEN_EXE" ]]; then
    printf 'missing texrender_offscreen executable: %s\n' "$OFFSCREEN_EXE" >&2
    exit 1
fi

if [[ ! -x "$SVG_EXE" ]]; then
    printf 'missing texrender_svg_export executable: %s\n' "$SVG_EXE" >&2
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

while IFS='|' read -r slug formula; do
    if [[ -z "${slug// }" || "${slug:0:1}" == "#" ]]; then
        continue
    fi
    if [[ -z "${formula:-}" ]]; then
        printf 'invalid gallery line for slug %s\n' "$slug" >&2
        exit 1
    fi

    "$OFFSCREEN_EXE" "$formula" "$OUTPUT_DIR/$slug.png" >/dev/null
    "$SVG_EXE" "$formula" "$OUTPUT_DIR/$slug.svg" >/dev/null
    printf '%s\n' "$slug"
done < "$FORMULAS_FILE"

printf 'gallery artifacts written to %s\n' "$OUTPUT_DIR"
