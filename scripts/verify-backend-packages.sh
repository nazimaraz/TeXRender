#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-/tmp/texgui-backend-packages}"
GENERATOR="${GENERATOR:-Ninja}"
BACKENDS="${TEXGUI_BACKEND_PACKAGE_BACKENDS:-raylib sdl2}"
REQUIRE_AVAILABLE="${TEXGUI_BACKEND_PACKAGE_REQUIRE:-0}"

run() {
    printf '\n==> %s\n' "$*"
    "$@"
}

has() {
    command -v "$1" >/dev/null 2>&1
}

cmake_dependency_available() {
    local package="$1"
    local probe_dir="${BUILD_ROOT}/probe-${package}"
    rm -rf "$probe_dir"
    mkdir -p "$probe_dir"
    cat > "${probe_dir}/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.21)
project(texgui_dependency_probe LANGUAGES CXX)
find_package(${package} REQUIRED)
EOF
    cmake -S "$probe_dir" -B "${probe_dir}/build" -G "$GENERATOR" >/dev/null 2>&1
}

verify_backend() {
    local backend="$1"
    local dependency_package="$2"
    local build_dir="${BUILD_ROOT}/${backend}/build"
    local prefix="${BUILD_ROOT}/${backend}/prefix"
    local header_consumer="${BUILD_ROOT}/${backend}/installed-headers"
    local backend_consumer="${BUILD_ROOT}/${backend}/backend-consumer"

    if ! cmake_dependency_available "$dependency_package"; then
        if [[ "$REQUIRE_AVAILABLE" == "1" ]]; then
            printf '\n%s package smoke requires find_package(%s), but it is not available\n' "$backend" "$dependency_package" >&2
            exit 1
        fi
        printf '\n==> skipping %s package smoke: find_package(%s) is not available\n' "$backend" "$dependency_package"
        return
    fi

    rm -rf "${BUILD_ROOT:?}/${backend}"
    run cmake -S "$ROOT_DIR" -B "$build_dir" -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DTEXGUI_BACKEND="$backend" \
        -DTEXGUI_INSTALL=ON \
        -DTEXGUI_BUILD_EXAMPLES=OFF \
        -DTEXGUI_BUILD_TESTS=OFF \
        -DTEXGUI_BUILD_BENCHMARKS=OFF
    run cmake --build "$build_dir" -j
    run cmake --install "$build_dir" --prefix "$prefix"

    run env \
        TEXGUI_HEADER_USE_EXISTING_INSTALL=1 \
        TEXGUI_HEADER_BACKEND="$backend" \
        TEXGUI_HEADER_INSTALL_PREFIX="$prefix" \
        TEXGUI_HEADER_CONSUMER_BUILD="$header_consumer" \
        "$ROOT_DIR/scripts/verify-installed-headers.sh"

    run cmake -S "$ROOT_DIR/tests/backend_package_consumer" -B "$backend_consumer" -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$prefix" \
        -DTEXGUI_BACKEND_PACKAGE="$backend"
    run cmake --build "$backend_consumer" -j
    run ctest --test-dir "$backend_consumer" --output-on-failure

    printf '\n%s installed backend package smoke completed successfully\n' "$backend"
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
mkdir -p "$BUILD_ROOT"

for backend in $BACKENDS; do
    case "$backend" in
        raylib)
            verify_backend raylib raylib
            ;;
        sdl2)
            verify_backend sdl2 SDL2
            ;;
        *)
            printf "unsupported backend package smoke target: '%s'\n" "$backend" >&2
            exit 1
            ;;
    esac
done

printf '\nbackend package verification completed successfully\n'
