#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-/tmp/texrender-installed-headers}"
GENERATOR="${GENERATOR:-Ninja}"
BACKEND="${TEXRENDER_HEADER_BACKEND:-headless}"
BUILD_DIR="${TEXRENDER_HEADER_BUILD_DIR:-${BUILD_ROOT}/build-${BACKEND}}"
INSTALL_PREFIX="${TEXRENDER_HEADER_INSTALL_PREFIX:-${BUILD_ROOT}/prefix-${BACKEND}}"
CONSUMER_BUILD="${TEXRENDER_HEADER_CONSUMER_BUILD:-${BUILD_ROOT}/consumer-${BACKEND}}"
USE_EXISTING_INSTALL="${TEXRENDER_HEADER_USE_EXISTING_INSTALL:-0}"

run() {
    printf '\n==> %s\n' "$*"
    "$@"
}

has() {
    command -v "$1" >/dev/null 2>&1
}

require_header() {
    local header="$1"
    if [[ ! -f "${INSTALL_PREFIX}/include/texrender/${header}" ]]; then
        printf 'expected installed header: include/texrender/%s\n' "$header" >&2
        exit 1
    fi
}

component_build_name() {
    local components="$1"
    printf '%s' "$components" | tr ';' '-'
}

expected_feature() {
    local feature_backend="$1"
    if [[ "$BACKEND" == "$feature_backend" ]]; then
        printf 'TRUE'
    else
        printf 'FALSE'
    fi
}

verify_components() {
    local components="$1"
    local build_name
    build_name="$(component_build_name "$components")"
    local build_dir="${BUILD_ROOT}/components-${BACKEND}-${build_name}"
    rm -rf "$build_dir"
    run cmake -S "$ROOT_DIR/tests/package_components" -B "$build_dir" -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$INSTALL_PREFIX" \
        "-DTEXRENDER_REQUIRED_COMPONENTS=${components}" \
        -DTEXRENDER_EXPECT_BACKEND="$BACKEND" \
        -DTEXRENDER_EXPECT_HAS_RAYLIB="$(expected_feature raylib)" \
        -DTEXRENDER_EXPECT_HAS_SDL2="$(expected_feature sdl2)"
    run cmake --build "$build_dir" -j
    run ctest --test-dir "$build_dir" --output-on-failure
}

expect_component_failure() {
    local component="$1"
    local build_dir="${BUILD_ROOT}/components-${BACKEND}-reject-${component}"
    rm -rf "$build_dir"
    mkdir -p "$build_dir"
    if cmake -S "$ROOT_DIR/tests/package_components" -B "$build_dir" -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$INSTALL_PREFIX" \
        -DTEXRENDER_REQUIRED_COMPONENTS="$component" \
        -DTEXRENDER_EXPECT_BACKEND="$BACKEND" \
        -DTEXRENDER_EXPECT_HAS_RAYLIB="$(expected_feature raylib)" \
        -DTEXRENDER_EXPECT_HAS_SDL2="$(expected_feature sdl2)" > "${build_dir}/configure.log" 2>&1
    then
        cat "${build_dir}/configure.log" >&2
        printf 'expected find_package(texrender COMPONENTS %s) to fail for %s package\n' "$component" "$BACKEND" >&2
        exit 1
    fi
    printf '\n==> expected component rejection succeeded: %s package rejected %s\n' "$BACKEND" "$component"
}

verify_backend_consumer() {
    local backend="$1"
    local build_dir="${BUILD_ROOT}/backend-consumer-${BACKEND}-${backend}"
    rm -rf "$build_dir"
    run cmake -S "$ROOT_DIR/tests/backend_package_consumer" -B "$build_dir" -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$INSTALL_PREFIX" \
        -DTEXRENDER_BACKEND_PACKAGE="$backend"
    run cmake --build "$build_dir" -j
    run ctest --test-dir "$build_dir" --output-on-failure
}

expect_backend_consumer_failure() {
    local backend="$1"
    local build_dir="${BUILD_ROOT}/backend-consumer-${BACKEND}-reject-${backend}"
    rm -rf "$build_dir"
    mkdir -p "$build_dir"
    if cmake -S "$ROOT_DIR/tests/backend_package_consumer" -B "$build_dir" -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$INSTALL_PREFIX" \
        -DTEXRENDER_BACKEND_PACKAGE="$backend" > "${build_dir}/configure.log" 2>&1
    then
        cat "${build_dir}/configure.log" >&2
        printf 'expected %s package to reject backend consumer for %s\n' "$BACKEND" "$backend" >&2
        exit 1
    fi
    printf '\n==> expected backend consumer rejection succeeded: %s package rejected %s consumer\n' "$BACKEND" "$backend"
}

forbid_header() {
    local header="$1"
    if [[ -f "${INSTALL_PREFIX}/include/texrender/${header}" ]]; then
        printf 'unexpected installed header for %s package: include/texrender/%s\n' "$BACKEND" "$header" >&2
        exit 1
    fi
}

require_doc() {
    local path="$1"
    if [[ ! -f "${INSTALL_PREFIX}/share/doc/texrender/${path}" ]]; then
        printf 'expected installed notice/documentation file: share/doc/texrender/%s\n' "$path" >&2
        exit 1
    fi
}

case "$BACKEND" in
    headless | svg | raylib | sdl2) ;;
    *)
        printf "unsupported TEXRENDER_HEADER_BACKEND='%s'\n" "$BACKEND" >&2
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

if [[ "$USE_EXISTING_INSTALL" != "1" ]]; then
    rm -rf "$BUILD_ROOT"
    mkdir -p "$BUILD_ROOT"
    run cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DTEXRENDER_BACKEND="$BACKEND" \
        -DTEXRENDER_INSTALL=ON \
        -DTEXRENDER_BUILD_EXAMPLES=OFF \
        -DTEXRENDER_BUILD_TESTS=OFF \
        -DTEXRENDER_BUILD_BENCHMARKS=OFF
    run cmake --build "$BUILD_DIR" -j
    run cmake --install "$BUILD_DIR" --prefix "$INSTALL_PREFIX"
else
    if [[ ! -d "$INSTALL_PREFIX/include/texrender" ]]; then
        printf 'existing install prefix does not contain include/texrender: %s\n' "$INSTALL_PREFIX" >&2
        exit 1
    fi
    rm -rf "$CONSUMER_BUILD"
fi

for header in \
    cache_options.hpp \
    config.hpp \
    draw_surface.hpp \
    export.hpp \
    formula_layout.hpp \
    glyph_bitmap.hpp \
    headless_surface.hpp \
    latex_document.hpp \
    render_handle.hpp \
    svg_surface.hpp \
    texrender.hpp \
    version.hpp
do
    require_header "$header"
done

for doc in \
    LICENSE \
    THIRD_PARTY.md \
    dependency-license-audit.md \
    upstream-microtex-patch.md
do
    require_doc "$doc"
done

case "$BACKEND" in
    raylib)
        require_header raylib_surface.hpp
        forbid_header sdl2_surface.hpp
        ;;
    sdl2)
        require_header sdl2_surface.hpp
        forbid_header raylib_surface.hpp
        ;;
    *)
        forbid_header raylib_surface.hpp
        forbid_header sdl2_surface.hpp
        ;;
esac

run cmake -S "$ROOT_DIR/tests/installed_headers" -B "$CONSUMER_BUILD" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$INSTALL_PREFIX" \
    -DTEXRENDER_INSTALLED_INCLUDE_DIR="$INSTALL_PREFIX/include"
run cmake --build "$CONSUMER_BUILD" -j
run ctest --test-dir "$CONSUMER_BUILD" --output-on-failure

verify_components 'headless;svg'
case "$BACKEND" in
    raylib)
        verify_components raylib
        verify_backend_consumer raylib
        expect_component_failure sdl2
        expect_backend_consumer_failure sdl2
        ;;
    sdl2)
        verify_components sdl2
        verify_backend_consumer sdl2
        expect_component_failure raylib
        expect_backend_consumer_failure raylib
        ;;
    *)
        expect_component_failure raylib
        expect_component_failure sdl2
        expect_backend_consumer_failure raylib
        expect_backend_consumer_failure sdl2
        ;;
esac

printf '\ninstalled-header smoke completed successfully\n'
printf 'backend: %s\n' "$BACKEND"
printf 'prefix: %s\n' "$INSTALL_PREFIX"
