#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-/tmp/texrender-release-asset}"
SOURCE_DIR="${TEXRENDER_ASSET_SOURCE_DIR:-$ROOT_DIR}"
ARCHIVE_PATH="${1:-${TEXRENDER_ASSET_ARCHIVE:-}}"
GENERATOR="${GENERATOR:-}"
BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"

run() {
    printf '\n==> %s\n' "$*"
    "$@"
}

has() {
    command -v "$1" >/dev/null 2>&1
}

if [[ -z "$ARCHIVE_PATH" ]]; then
    printf 'usage: %s <release-asset.tar.gz|release-asset.zip>\n' "$0" >&2
    exit 1
fi

if [[ ! -f "$ARCHIVE_PATH" ]]; then
    printf 'release asset does not exist: %s\n' "$ARCHIVE_PATH" >&2
    exit 1
fi

if ! has cmake; then
    printf 'cmake is required\n' >&2
    exit 1
fi

ASSET_NAME="$(basename "$ARCHIVE_PATH")"
ASSET_STEM="$ASSET_NAME"
ASSET_STEM="${ASSET_STEM%.tar.gz}"
ASSET_STEM="${ASSET_STEM%.tgz}"
ASSET_STEM="${ASSET_STEM%.zip}"
EXTRACT_DIR="${BUILD_ROOT}/extract-${ASSET_STEM}"
CONSUMER_BUILD="${BUILD_ROOT}/consumer-${ASSET_STEM}"

rm -rf "$EXTRACT_DIR" "$CONSUMER_BUILD"
mkdir -p "$EXTRACT_DIR" "$CONSUMER_BUILD"

SHA_FILE="${ARCHIVE_PATH}.sha256"
if [[ -f "$SHA_FILE" ]]; then
    ARCHIVE_DIR="$(cd "$(dirname "$ARCHIVE_PATH")" && pwd)"
    SHA_BASENAME="$(basename "$SHA_FILE")"
    if has shasum; then
        run cmake -E chdir "$ARCHIVE_DIR" shasum -a 256 -c "$SHA_BASENAME"
    elif has sha256sum; then
        run cmake -E chdir "$ARCHIVE_DIR" sha256sum -c "$SHA_BASENAME"
    else
        printf 'warning: checksum file exists but no sha256 tool was found\n' >&2
    fi
fi

run cmake -E chdir "$EXTRACT_DIR" cmake -E tar xf "$ARCHIVE_PATH"

extracted_count=0
INSTALL_PREFIX=""
while IFS= read -r extracted_root; do
    extracted_count=$((extracted_count + 1))
    INSTALL_PREFIX="$extracted_root"
done < <(find "$EXTRACT_DIR" -mindepth 1 -maxdepth 1 -type d | sort)
if [[ "$extracted_count" -ne 1 ]]; then
    printf 'expected exactly one top-level directory in %s, found %s\n' "$ARCHIVE_PATH" "$extracted_count" >&2
    find "$EXTRACT_DIR" -mindepth 1 -maxdepth 1 -print >&2
    exit 1
fi

for required in \
    "$INSTALL_PREFIX/include/texrender/texrender.hpp" \
    "$INSTALL_PREFIX/lib/cmake/texrender/texrenderConfig.cmake" \
    "$INSTALL_PREFIX/share/texrender/res/.clatexmath-res_root"
do
    if [[ ! -e "$required" ]]; then
        printf 'release asset is missing expected installed file: %s\n' "$required" >&2
        exit 1
    fi
done

configure_args=(
    -S "$SOURCE_DIR/tests/package_consumer"
    -B "$CONSUMER_BUILD"
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DCMAKE_PREFIX_PATH="$INSTALL_PREFIX"
)
if [[ -n "$GENERATOR" ]]; then
    configure_args=(-G "$GENERATOR" "${configure_args[@]}")
fi

run cmake "${configure_args[@]}"
run cmake --build "$CONSUMER_BUILD" --config "$BUILD_TYPE" -j
run ctest --test-dir "$CONSUMER_BUILD" -C "$BUILD_TYPE" --output-on-failure

printf '\nrelease asset consumer smoke completed successfully\n'
printf 'asset: %s\n' "$ARCHIVE_PATH"
printf 'prefix: %s\n' "$INSTALL_PREFIX"
