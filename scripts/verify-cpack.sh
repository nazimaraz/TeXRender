#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-/tmp/texrender-cpack}"
GENERATOR="${GENERATOR:-Ninja}"
BUILD_DIR="${BUILD_ROOT}/build"
ARTIFACT_DIR="${BUILD_ROOT}/artifacts"
BINARY_EXTRACT="${BUILD_ROOT}/binary-extract"
SOURCE_EXTRACT="${BUILD_ROOT}/source-extract"
CONSUMER_BUILD="${BUILD_ROOT}/binary-consumer"
SOURCE_BUILD="${BUILD_ROOT}/source-build"

run() {
    printf '\n==> %s\n' "$*"
    "$@"
}

rm -rf "$BUILD_ROOT"
mkdir -p "$ARTIFACT_DIR" "$BINARY_EXTRACT" "$SOURCE_EXTRACT"

run cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DTEXRENDER_BACKEND=headless \
    -DTEXRENDER_BUILD_EXAMPLES=OFF \
    -DTEXRENDER_BUILD_TESTS=OFF \
    -DTEXRENDER_BUILD_BENCHMARKS=OFF
run cmake --build "$BUILD_DIR" -j

run cpack --config "$BUILD_DIR/CPackConfig.cmake" -G TGZ -B "$ARTIFACT_DIR"
run cpack --config "$BUILD_DIR/CPackSourceConfig.cmake" -G TGZ -B "$ARTIFACT_DIR"

BINARY_PACKAGE="$(find "$ARTIFACT_DIR" -maxdepth 1 -name 'TeXRender-*.tar.gz' ! -name '*source*' | head -n 1)"
SOURCE_PACKAGE="$(find "$ARTIFACT_DIR" -maxdepth 1 -name 'TeXRender-*-source.tar.gz' | head -n 1)"

if [[ -z "$BINARY_PACKAGE" || -z "$SOURCE_PACKAGE" ]]; then
    printf 'expected binary and source CPack archives in %s\n' "$ARTIFACT_DIR" >&2
    exit 1
fi

run tar -xzf "$BINARY_PACKAGE" -C "$BINARY_EXTRACT"
BINARY_PREFIX="$(find "$BINARY_EXTRACT" -mindepth 1 -maxdepth 1 -type d | head -n 1)"

run cmake -S "$ROOT_DIR/tests/package_consumer" -B "$CONSUMER_BUILD" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$BINARY_PREFIX"
run cmake --build "$CONSUMER_BUILD" -j
run ctest --test-dir "$CONSUMER_BUILD" --output-on-failure
run env \
    TEXRENDER_HEADER_USE_EXISTING_INSTALL=1 \
    TEXRENDER_HEADER_BACKEND=headless \
    TEXRENDER_HEADER_INSTALL_PREFIX="$BINARY_PREFIX" \
    TEXRENDER_HEADER_CONSUMER_BUILD="${BUILD_ROOT}/binary-installed-headers" \
    "$ROOT_DIR/scripts/verify-installed-headers.sh"

run tar -xzf "$SOURCE_PACKAGE" -C "$SOURCE_EXTRACT"
SOURCE_DIR="$(find "$SOURCE_EXTRACT" -mindepth 1 -maxdepth 1 -type d | head -n 1)"
run cmake -S "$SOURCE_DIR" -B "$SOURCE_BUILD" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DTEXRENDER_BACKEND=headless \
    -DTEXRENDER_BUILD_EXAMPLES=OFF \
    -DTEXRENDER_BUILD_TESTS=OFF \
    -DTEXRENDER_BUILD_BENCHMARKS=OFF
run cmake --build "$SOURCE_BUILD" -j

printf '\nCPack smoke completed successfully\n'
printf 'binary package: %s\n' "$BINARY_PACKAGE"
printf 'source package: %s\n' "$SOURCE_PACKAGE"
