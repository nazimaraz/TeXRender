#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-/tmp/texrender-release-archive}"
GENERATOR="${GENERATOR:-Ninja}"
SOURCE_REF="${TEXRENDER_ARCHIVE_REF:-HEAD}"
VERSION="${TEXRENDER_ARCHIVE_VERSION:-$(git -C "$ROOT_DIR" describe --tags --always --dirty)}"
PREFIX_NAME="TeXRender-${VERSION#v}"
ARCHIVE_DIR="${BUILD_ROOT}/artifacts"
EXTRACT_DIR="${BUILD_ROOT}/src"
ARCHIVE="${ARCHIVE_DIR}/${PREFIX_NAME}.tar.gz"
SHA_FILE="${ARCHIVE}.sha256"
BUILD_DIR="${BUILD_ROOT}/build"
INSTALL_PREFIX="${BUILD_ROOT}/prefix"
CONSUMER_BUILD="${BUILD_ROOT}/consumer"

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

rm -rf "$BUILD_ROOT"
mkdir -p "$ARCHIVE_DIR" "$EXTRACT_DIR"

run git -C "$ROOT_DIR" archive --format=tar.gz --prefix="${PREFIX_NAME}/" "$SOURCE_REF" -o "$ARCHIVE"
if has shasum; then
    run shasum -a 256 "$ARCHIVE"
    shasum -a 256 "$ARCHIVE" > "$SHA_FILE"
elif has sha256sum; then
    run sha256sum "$ARCHIVE"
    sha256sum "$ARCHIVE" > "$SHA_FILE"
else
    printf 'warning: no sha256 tool found; skipping checksum file\n' >&2
fi

run tar -xzf "$ARCHIVE" -C "$EXTRACT_DIR"
SOURCE_DIR="${EXTRACT_DIR}/${PREFIX_NAME}"

run cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DTEXRENDER_BACKEND=headless \
    -DTEXRENDER_WARNINGS_AS_ERRORS=ON
run cmake --build "$BUILD_DIR" -j
run ctest --test-dir "$BUILD_DIR" --output-on-failure

run "$BUILD_DIR/texrender_offscreen" 'e^{i\pi} + 1 = 0' "${BUILD_ROOT}/formula.png"
run cmake --install "$BUILD_DIR" --prefix "$INSTALL_PREFIX"
run cmake -S "$SOURCE_DIR/tests/package_consumer" -B "$CONSUMER_BUILD" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$INSTALL_PREFIX"
run cmake --build "$CONSUMER_BUILD" -j
run ctest --test-dir "$CONSUMER_BUILD" --output-on-failure
run env \
    BUILD_ROOT="${BUILD_ROOT}/installed-header-smoke" \
    TEXRENDER_HEADER_USE_EXISTING_INSTALL=1 \
    TEXRENDER_HEADER_BACKEND=headless \
    TEXRENDER_HEADER_INSTALL_PREFIX="$INSTALL_PREFIX" \
    TEXRENDER_HEADER_CONSUMER_BUILD="${BUILD_ROOT}/installed-headers" \
    "$SOURCE_DIR/scripts/verify-installed-headers.sh"

printf '\nrelease archive smoke completed successfully\n'
printf 'archive: %s\n' "$ARCHIVE"
if [[ -f "$SHA_FILE" ]]; then
    printf 'sha256: %s\n' "$SHA_FILE"
fi
