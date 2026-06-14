#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-/tmp/texrender-asan}"
GENERATOR="${GENERATOR:-Ninja}"
CXX_COMPILER="${CXX:-}"

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

PROBE_DIR="${BUILD_DIR}/asan-probe"
rm -rf "$PROBE_DIR"
mkdir -p "$PROBE_DIR"
cat > "$PROBE_DIR/probe.cpp" <<'EOF'
#include <sstream>

int main()
{
    auto stream = std::ostringstream{};
    stream << 1;
    return stream.str() == "1" ? 0 : 1;
}
EOF

SDK_FLAGS=()
CMAKE_SDK_ARG=()
if [[ "$(uname -s)" == "Darwin" ]]; then
    SANITIZER_SDKROOT="${SDKROOT:-}"
    if [[ -z "$SANITIZER_SDKROOT" ]] && command -v xcrun >/dev/null 2>&1; then
        SANITIZER_SDKROOT="$(xcrun --sdk macosx --show-sdk-path 2>/dev/null || xcrun --show-sdk-path 2>/dev/null || true)"
    fi
    if [[ -n "$SANITIZER_SDKROOT" ]]; then
        SDK_FLAGS=(-isysroot "$SANITIZER_SDKROOT")
        CMAKE_SDK_ARG=(-DCMAKE_OSX_SYSROOT="$SANITIZER_SDKROOT")
    fi
fi

if [[ -n "$CXX_COMPILER" ]]; then
    CXX_CANDIDATES=("$CXX_COMPILER")
else
    CXX_CANDIDATES=(
        clang++
        /opt/homebrew/opt/llvm/bin/clang++
        /usr/local/opt/llvm/bin/clang++
        clang++-22
        clang++-21
        clang++-20
        clang++-19
        clang++-18
    )
fi

SELECTED_CXX=""
SELECTED_LINK_FLAGS=()
ATTEMPT=0
FAILURE_LOG="${PROBE_DIR}/failures.log"
: > "$FAILURE_LOG"
for candidate in "${CXX_CANDIDATES[@]}"; do
    if ! command -v "$candidate" >/dev/null 2>&1; then
        continue
    fi

    ATTEMPT=$((ATTEMPT + 1))
    candidate_path="$(command -v "$candidate")"
    candidate_prefix="$(cd "$(dirname "$candidate_path")/.." && pwd)"
    candidate_link_flags=()
    if [[ -f "${candidate_prefix}/lib/c++/libc++.dylib" ]]; then
        candidate_link_flags=("-L${candidate_prefix}/lib/c++" "-Wl,-rpath,${candidate_prefix}/lib/c++")
    fi
    build_log="${PROBE_DIR}/build-${ATTEMPT}.log"
    run_log="${PROBE_DIR}/run-${ATTEMPT}.log"
    probe_bin="${PROBE_DIR}/probe-${ATTEMPT}"
    if ! "$candidate_path" "${SDK_FLAGS[@]}" -fsanitize=address,undefined -fno-omit-frame-pointer \
        "$PROBE_DIR/probe.cpp" "${candidate_link_flags[@]}" -o "$probe_bin" > "$build_log" 2>&1; then
        {
            printf '\n==> %s could not build an ASan/UBSan probe\n' "$candidate_path"
            cat "$build_log"
        } >> "$FAILURE_LOG"
        continue
    fi

    if ! /bin/sh -c '"$1"' sh "$probe_bin" > "$run_log" 2>&1; then
        {
            printf '\n==> %s built the probe, but its ASan runtime could not run\n' "$candidate_path"
            cat "$run_log"
        } >> "$FAILURE_LOG"
        continue
    fi

    SELECTED_CXX="$candidate_path"
    SELECTED_LINK_FLAGS=("${candidate_link_flags[@]}")
    break
done

if [[ -z "$SELECTED_CXX" ]]; then
    cat "$FAILURE_LOG" >&2
    printf '\nNo available compiler could build and run an ASan/UBSan probe.\n' >&2
    printf 'Set CXX to a Clang compiler with a compatible AddressSanitizer runtime, for example Homebrew/LLVM clang.\n' >&2
    exit 1
fi

cd "$ROOT_DIR"

printf 'Using ASan/UBSan compiler: %s\n' "$SELECTED_CXX"
SANITIZER_LINK_FLAGS="-fsanitize=address,undefined"
for flag in "${SELECTED_LINK_FLAGS[@]}"; do
    SANITIZER_LINK_FLAGS="${SANITIZER_LINK_FLAGS} ${flag}"
done
SANITIZER_LINK_FLAGS="${SANITIZER_LINK_FLAGS} ${CMAKE_EXE_LINKER_FLAGS:-}"

run cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_CXX_COMPILER="$SELECTED_CXX" \
    "${CMAKE_SDK_ARG[@]}" \
    -DTEXRENDER_BACKEND=headless \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -gdwarf-4 ${CMAKE_CXX_FLAGS:-}" \
    -DCMAKE_EXE_LINKER_FLAGS="$SANITIZER_LINK_FLAGS"

run cmake --build "$BUILD_DIR" -j

export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"
export UBSAN_OPTIONS="${UBSAN_OPTIONS:-print_stacktrace=1}"
run ctest --test-dir "$BUILD_DIR" --output-on-failure

printf '\nAddressSanitizer/UBSan smoke completed successfully\n'
