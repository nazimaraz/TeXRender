#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-/tmp/texrender-tsan}"
GENERATOR="${GENERATOR:-Ninja}"
CXX_COMPILER="${CXX:-clang++}"

run() {
    printf '\n==> %s\n' "$*"
    "$@"
}

if ! command -v cmake >/dev/null 2>&1; then
    printf 'cmake is required\n' >&2
    exit 1
fi

if [[ "$GENERATOR" == "Ninja" ]] && ! command -v ninja >/dev/null 2>&1; then
    printf 'ninja is required for GENERATOR=Ninja\n' >&2
    exit 1
fi

if ! command -v "$CXX_COMPILER" >/dev/null 2>&1; then
    printf "C++ compiler '%s' is not available\n" "$CXX_COMPILER" >&2
    printf 'Set CXX to a Clang compiler with ThreadSanitizer support, for example CXX=clang++-22.\n' >&2
    exit 1
fi

cd "$ROOT_DIR"

run cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_CXX_COMPILER="$CXX_COMPILER" \
    -DTEXRENDER_BACKEND=headless \
    -DTEXRENDER_BUILD_EXAMPLES=OFF \
    -DTEXRENDER_BUILD_BENCHMARKS=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer ${CMAKE_CXX_FLAGS:-}" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread ${CMAKE_EXE_LINKER_FLAGS:-}"

run cmake --build "$BUILD_DIR" --target texrender_lifetime_stress_test texrender_headless_render_test -j

export TSAN_OPTIONS="${TSAN_OPTIONS:-halt_on_error=1}"
run "$BUILD_DIR/texrender_lifetime_stress_test"
run "$BUILD_DIR/texrender_headless_render_test"

printf '\nThreadSanitizer smoke completed successfully\n'
