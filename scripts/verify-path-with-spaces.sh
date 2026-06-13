#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-/tmp/texgui path smoke}"
GENERATOR="${GENERATOR:-Ninja}"
SOURCE_REF="${TEXGUI_ARCHIVE_REF:-HEAD}"
SOURCE_ARCHIVE="${BUILD_ROOT}/source.tar"
SOURCE_DIR="${BUILD_ROOT}/source tree"
BUILD_DIR="${BUILD_ROOT}/build tree"
INSTALL_PREFIX="${BUILD_ROOT}/install prefix"
CONSUMER_BUILD="${BUILD_ROOT}/consumer build"

run() {
    printf '\n==> %s\n' "$*"
    "$@"
}

if [[ "$GENERATOR" == "Ninja" ]] && ! command -v ninja >/dev/null 2>&1; then
    printf 'ninja is required for GENERATOR=Ninja\n' >&2
    exit 1
fi

rm -rf "$BUILD_ROOT"
mkdir -p "$SOURCE_DIR"

run git -C "$ROOT_DIR" archive --format=tar "$SOURCE_REF" -o "$SOURCE_ARCHIVE"
run tar -xf "$SOURCE_ARCHIVE" -C "$SOURCE_DIR"

run cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DTEXGUI_BACKEND=headless \
    -DTEXGUI_WARNINGS_AS_ERRORS=ON
run cmake --build "$BUILD_DIR" -j
run ctest --test-dir "$BUILD_DIR" --output-on-failure
run "$BUILD_DIR/texgui_offscreen" 'e^{i\pi} + 1 = 0' "${BUILD_ROOT}/formula output.png"

run cmake --install "$BUILD_DIR" --prefix "$INSTALL_PREFIX"
run cmake -S "$SOURCE_DIR/tests/package_consumer" -B "$CONSUMER_BUILD" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$INSTALL_PREFIX"
run cmake --build "$CONSUMER_BUILD" -j
run ctest --test-dir "$CONSUMER_BUILD" --output-on-failure

printf '\npath-with-spaces smoke completed successfully\n'
