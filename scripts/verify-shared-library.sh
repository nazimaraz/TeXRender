#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-/tmp/texgui-shared-library}"
GENERATOR="${GENERATOR:-Ninja}"
BACKEND="${TEXGUI_SHARED_BACKEND:-headless}"
BUILD_DIR="${BUILD_ROOT}/build-${BACKEND}"
INSTALL_PREFIX="${BUILD_ROOT}/prefix-${BACKEND}"
PACKAGE_CONSUMER_BUILD="${BUILD_ROOT}/package-consumer-${BACKEND}"
FIND_EXAMPLE_BUILD="${BUILD_ROOT}/find-example-${BACKEND}"

run() {
    printf '\n==> %s\n' "$*"
    "$@"
}

has() {
    command -v "$1" >/dev/null 2>&1
}

runtime_path_var() {
    case "$(uname -s)" in
        Darwin) printf 'DYLD_LIBRARY_PATH' ;;
        MINGW* | MSYS* | CYGWIN*) printf 'PATH' ;;
        *) printf 'LD_LIBRARY_PATH' ;;
    esac
}

runtime_dirs() {
    case "$(uname -s)" in
        MINGW* | MSYS* | CYGWIN*) printf '%s/bin:%s/lib' "$INSTALL_PREFIX" "$INSTALL_PREFIX" ;;
        *) printf '%s/lib' "$INSTALL_PREFIX" ;;
    esac
}

run_with_shared_runtime() {
    local var
    var="$(runtime_path_var)"
    local dirs
    dirs="$(runtime_dirs)"
    local current="${!var:-}"
    if [[ -n "$current" ]]; then
        dirs="${dirs}:${current}"
    fi
    run cmake -E env "${var}=${dirs}" "$@"
}

require_shared_library() {
    if find "$INSTALL_PREFIX" -type f \( \
        -name 'libtexgui.so' -o \
        -name 'libtexgui.so.*' -o \
        -name 'libtexgui.dylib' -o \
        -name 'texgui.dll' \
        \) | grep -q .; then
        return
    fi

    printf 'expected an installed shared texgui library under %s\n' "$INSTALL_PREFIX" >&2
    find "$INSTALL_PREFIX" -maxdepth 3 -type f >&2 || true
    exit 1
}

case "$BACKEND" in
    headless | svg)
        ;;
    *)
        printf "unsupported TEXGUI_SHARED_BACKEND='%s'; use headless or svg for dependency-free shared smoke\n" "$BACKEND" >&2
        exit 1
        ;;
esac

if ! has cmake; then
    printf 'cmake is required\n' >&2
    exit 1
fi

if [[ "$GENERATOR" == "Ninja" ]] && ! has ninja; then
    printf 'ninja is required for GENERATOR=Ninja\n' >&2
    exit 1
fi

rm -rf "$BUILD_ROOT"
mkdir -p "$BUILD_ROOT"

run cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DTEXGUI_BACKEND="$BACKEND" \
    -DTEXGUI_INSTALL=ON \
    -DTEXGUI_BUILD_EXAMPLES=OFF \
    -DTEXGUI_BUILD_BENCHMARKS=OFF
run cmake --build "$BUILD_DIR" -j
run_with_shared_runtime ctest --test-dir "$BUILD_DIR" --output-on-failure
run cmake --install "$BUILD_DIR" --prefix "$INSTALL_PREFIX"
require_shared_library

run env \
    TEXGUI_HEADER_USE_EXISTING_INSTALL=1 \
    TEXGUI_HEADER_BACKEND="$BACKEND" \
    TEXGUI_HEADER_INSTALL_PREFIX="$INSTALL_PREFIX" \
    TEXGUI_HEADER_CONSUMER_BUILD="${BUILD_ROOT}/installed-headers-${BACKEND}" \
    "$ROOT_DIR/scripts/verify-installed-headers.sh"

run cmake -S "$ROOT_DIR/tests/package_consumer" -B "$PACKAGE_CONSUMER_BUILD" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$INSTALL_PREFIX"
run cmake --build "$PACKAGE_CONSUMER_BUILD" -j
run_with_shared_runtime ctest --test-dir "$PACKAGE_CONSUMER_BUILD" --output-on-failure

run cmake -S "$ROOT_DIR/examples/consumer-find-package" -B "$FIND_EXAMPLE_BUILD" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$INSTALL_PREFIX"
run cmake --build "$FIND_EXAMPLE_BUILD" -j
run_with_shared_runtime "$FIND_EXAMPLE_BUILD/texgui_find_package_example"

printf '\nshared-library package smoke completed successfully\n'
printf 'backend: %s\n' "$BACKEND"
printf 'prefix: %s\n' "$INSTALL_PREFIX"
