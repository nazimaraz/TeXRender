#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VCPKG_ROOT="${VCPKG_ROOT:-}"
TRIPLET="${VCPKG_DEFAULT_TRIPLET:-}"
PACKAGE_SPEC="${TEXGUI_VCPKG_PACKAGE_SPEC:-${1:-latex-renderer}}"
COMPONENT_BUILD="${TEXGUI_VCPKG_COMPONENT_BUILD:-/tmp/texgui-vcpkg-components}"
GENERATOR="${GENERATOR:-Ninja}"

component_build_name() {
    printf '%s' "$1" | tr ';' '-'
}

configure_component_smoke() {
    local components="$1"
    local build_dir="$2"
    local backend="$3"
    local expect_raylib="$4"
    local expect_sdl2="$5"

    rm -rf "$build_dir"
    cmake -S "$ROOT_DIR/tests/package_components" -B "$build_dir" -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
        -DVCPKG_TARGET_TRIPLET="$TRIPLET" \
        "-DTEXGUI_REQUIRED_COMPONENTS=${components}" \
        -DTEXGUI_EXPECT_BACKEND="$backend" \
        -DTEXGUI_EXPECT_HAS_RAYLIB="$expect_raylib" \
        -DTEXGUI_EXPECT_HAS_SDL2="$expect_sdl2"
}

verify_component_success() {
    local components="$1"
    local backend="$2"
    local expect_raylib="$3"
    local expect_sdl2="$4"
    local build_dir="${COMPONENT_BUILD}-$(component_build_name "$components")"

    configure_component_smoke "$components" "$build_dir" "$backend" "$expect_raylib" "$expect_sdl2"
    cmake --build "$build_dir" -j
    ctest --test-dir "$build_dir" --output-on-failure
}

expect_component_failure() {
    local component="$1"
    local backend="$2"
    local expect_raylib="$3"
    local expect_sdl2="$4"
    local build_dir="${COMPONENT_BUILD}-reject-${component}"

    rm -rf "$build_dir"
    mkdir -p "$build_dir"
    if configure_component_smoke "$component" "$build_dir" "$backend" "$expect_raylib" "$expect_sdl2" \
        > "${build_dir}/configure.log" 2>&1; then
        cat "${build_dir}/configure.log" >&2
        printf 'expected vcpkg package %s to reject texgui component %s\n' "$PACKAGE_SPEC" "$component" >&2
        exit 1
    fi

    printf '\n==> expected vcpkg component rejection succeeded: %s rejected %s\n' "$PACKAGE_SPEC" "$component"
}

if [[ -n "$VCPKG_ROOT" && -d "$VCPKG_ROOT" ]]; then
    VCPKG_ROOT="$(cd "$VCPKG_ROOT" && pwd -P)"
fi

if [[ -z "$VCPKG_ROOT" || ! -x "$VCPKG_ROOT/vcpkg" ]]; then
    printf 'VCPKG_ROOT must point at a bootstrapped vcpkg checkout\n' >&2
    exit 1
fi

if [[ "$GENERATOR" == "Ninja" ]] && ! command -v ninja >/dev/null 2>&1; then
    printf 'ninja is required for GENERATOR=Ninja\n' >&2
    exit 1
fi

if [[ -z "$TRIPLET" ]]; then
    case "$(uname -s)" in
        Darwin)
            case "$(uname -m)" in
                arm64|aarch64) TRIPLET="arm64-osx" ;;
                *) TRIPLET="x64-osx" ;;
            esac
            ;;
        Linux) TRIPLET="x64-linux" ;;
        MINGW*|MSYS*|CYGWIN*) TRIPLET="x64-windows" ;;
        *) TRIPLET="x64-linux" ;;
    esac
fi

export TEXGUI_VCPKG_SOURCE_DIR="$ROOT_DIR"

"$VCPKG_ROOT/vcpkg" install "$PACKAGE_SPEC" \
    --classic \
    --recurse \
    --overlay-ports="$ROOT_DIR/packaging/vcpkg-overlay/ports" \
    --triplet="$TRIPLET"

backend="headless"
expect_raylib="FALSE"
expect_sdl2="FALSE"
required_components="headless;svg"
case "$PACKAGE_SPEC" in
    *"[raylib]"*)
        backend="raylib"
        expect_raylib="TRUE"
        required_components="raylib"
        ;;
    *"[sdl2]"*)
        backend="sdl2"
        expect_sdl2="TRUE"
        required_components="sdl2"
        ;;
esac

verify_component_success "$required_components" "$backend" "$expect_raylib" "$expect_sdl2"
case "$backend" in
    raylib)
        expect_component_failure sdl2 "$backend" "$expect_raylib" "$expect_sdl2"
        ;;
    sdl2)
        expect_component_failure raylib "$backend" "$expect_raylib" "$expect_sdl2"
        ;;
    *)
        expect_component_failure raylib "$backend" "$expect_raylib" "$expect_sdl2"
        expect_component_failure sdl2 "$backend" "$expect_raylib" "$expect_sdl2"
        ;;
esac
