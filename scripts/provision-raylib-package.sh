#!/usr/bin/env bash
set -euo pipefail

VERSION="${RAYLIB_VERSION:-5.5}"
BUILD_ROOT="${BUILD_ROOT:-/tmp/texrender-raylib-package}"
INSTALL_PREFIX="${RAYLIB_INSTALL_PREFIX:-${BUILD_ROOT}/prefix}"
GENERATOR="${GENERATOR:-Ninja}"
SOURCE_DIR="${BUILD_ROOT}/src"
BUILD_DIR="${BUILD_ROOT}/build"

run() {
    printf '\n==> %s\n' "$*"
    "$@"
}

has() {
    command -v "$1" >/dev/null 2>&1
}

if ! has cmake; then
    printf 'cmake is required\n' >&2
    exit 1
fi

if [[ "$GENERATOR" == "Ninja" ]] && ! has ninja; then
    printf 'ninja is required for GENERATOR=Ninja\n' >&2
    exit 1
fi

if ! has git; then
    printf 'git is required\n' >&2
    exit 1
fi

rm -rf "$BUILD_ROOT"
mkdir -p "$BUILD_ROOT"

run git clone --depth 1 --branch "$VERSION" https://github.com/raysan5/raylib.git "$SOURCE_DIR"
run cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    -DBUILD_EXAMPLES=OFF
run cmake --build "$BUILD_DIR" --target install -j

printf '\nraylib CMake package installed successfully\n'
printf 'version: %s\n' "$VERSION"
printf 'prefix: %s\n' "$INSTALL_PREFIX"
