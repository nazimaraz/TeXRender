#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-/tmp/texrender-verify-release}"
GENERATOR="${GENERATOR:-Ninja}"
HEADLESS_BUILD="${BUILD_ROOT}/headless"
RAYLIB_BUILD="${BUILD_ROOT}/raylib"
SDL2_BUILD="${BUILD_ROOT}/sdl2"
SHARED_BUILD="${BUILD_ROOT}/headless-shared"
SVG_BUILD="${BUILD_ROOT}/svg"
INSTALL_PREFIX="${BUILD_ROOT}/prefix"
PACKAGE_CONSUMER_BUILD="${BUILD_ROOT}/package-consumer"
FETCH_EXAMPLE_BUILD="${BUILD_ROOT}/fetch-example"
FIND_EXAMPLE_BUILD="${BUILD_ROOT}/find-example"

run() {
    printf '\n==> %s\n' "$*"
    "$@"
}

has() {
    command -v "$1" >/dev/null 2>&1
}

cmake_configure() {
    local source="$1"
    local build="$2"
    shift 2
    run cmake -S "$source" -B "$build" -G "$GENERATOR" "$@"
}

cmake_build() {
    local build="$1"
    shift
    run cmake --build "$build" -j "$@"
}

cd "$ROOT_DIR"

printf 'texrender release verification\n'
printf 'root: %s\n' "$ROOT_DIR"
printf 'build root: %s\n' "$BUILD_ROOT"
mkdir -p "$BUILD_ROOT"

if ! has cmake; then
    printf 'cmake is required\n' >&2
    exit 1
fi

if [[ "$GENERATOR" == "Ninja" ]] && ! has ninja; then
    printf 'ninja is required for GENERATOR=Ninja\n' >&2
    exit 1
fi

if has clang-format; then
    printf '\n==> clang-format --dry-run\n'
    git ls-files '*.cpp' '*.hpp' | xargs clang-format --dry-run -Werror
else
    printf '\n==> skipping clang-format: not installed\n'
fi

run git diff --check

if has python3; then
    run python3 "$ROOT_DIR/scripts/check-doc-links.py"
    run python3 "$ROOT_DIR/scripts/check-registry-readiness.py"
    run python3 "$ROOT_DIR/scripts/check-third-party-notices.py"
    run python3 "$ROOT_DIR/scripts/check-public-api.py"
else
    printf '\n==> skipping Python-based docs/registry checks: python3 is not installed\n'
fi

cmake_configure "$ROOT_DIR" "$HEADLESS_BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DTEXRENDER_BACKEND=headless \
    -DTEXRENDER_WARNINGS_AS_ERRORS=ON
cmake_build "$HEADLESS_BUILD"
run ctest --test-dir "$HEADLESS_BUILD" --output-on-failure
run "$HEADLESS_BUILD/texrender_offscreen" 'e^{i\pi} + 1 = 0' "${BUILD_ROOT}/formula.png"
run "$HEADLESS_BUILD/texrender_custom_surface"
run env TEXRENDER_BUILD_DIR="$HEADLESS_BUILD" "$ROOT_DIR/scripts/generate-gallery.sh" \
    "$ROOT_DIR/docs/gallery-formulas.txt" "${BUILD_ROOT}/gallery"
if has ffmpeg; then
    run env GALLERY_DIR="${BUILD_ROOT}/gallery" "$ROOT_DIR/scripts/generate-preview-gif.sh" "${BUILD_ROOT}/live-preview.gif"
else
    printf '\n==> skipping README GIF generation: ffmpeg is not installed\n'
fi
run "$HEADLESS_BUILD/texrender_render_benchmark"
run "$HEADLESS_BUILD/texrender_render_benchmark" > "${BUILD_ROOT}/benchmark-results.txt"
if has python3; then
    run python3 "$ROOT_DIR/scripts/check-benchmark-baseline.py" --input "${BUILD_ROOT}/benchmark-results.txt" --warn-only
fi
run cmake -E chdir "$BUILD_ROOT" "$HEADLESS_BUILD/texrender_headless_batch" 'e^{i\pi} + 1 = 0'

rm -rf "$SVG_BUILD"
cmake_configure "$ROOT_DIR" "$SVG_BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DTEXRENDER_BACKEND=svg
cmake_build "$SVG_BUILD"
run ctest --test-dir "$SVG_BUILD" --output-on-failure
run "$SVG_BUILD/texrender_svg_export" 'e^{i\pi} + 1 = 0' "${BUILD_ROOT}/formula.svg"

rm -rf "$SHARED_BUILD"
cmake_configure "$ROOT_DIR" "$SHARED_BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DTEXRENDER_BACKEND=headless \
    -DTEXRENDER_BUILD_EXAMPLES=OFF \
    -DTEXRENDER_BUILD_BENCHMARKS=OFF
cmake_build "$SHARED_BUILD"
run ctest --test-dir "$SHARED_BUILD" --output-on-failure

if [[ "${TEXRENDER_SKIP_SHARED_LIBRARY_SMOKE:-0}" != "1" ]]; then
    run env BUILD_ROOT="${BUILD_ROOT}/shared-library" "$ROOT_DIR/scripts/verify-shared-library.sh"
fi

rm -rf "$INSTALL_PREFIX" "$PACKAGE_CONSUMER_BUILD" "$FIND_EXAMPLE_BUILD"
run cmake --install "$HEADLESS_BUILD" --prefix "$INSTALL_PREFIX"
cmake_configure "$ROOT_DIR/tests/package_consumer" "$PACKAGE_CONSUMER_BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$INSTALL_PREFIX"
cmake_build "$PACKAGE_CONSUMER_BUILD"
run ctest --test-dir "$PACKAGE_CONSUMER_BUILD" --output-on-failure
run env \
    TEXRENDER_HEADER_USE_EXISTING_INSTALL=1 \
    TEXRENDER_HEADER_BACKEND=headless \
    TEXRENDER_HEADER_INSTALL_PREFIX="$INSTALL_PREFIX" \
    TEXRENDER_HEADER_CONSUMER_BUILD="${BUILD_ROOT}/installed-headers" \
    "$ROOT_DIR/scripts/verify-installed-headers.sh"

cmake_configure "$ROOT_DIR/examples/consumer-find-package" "$FIND_EXAMPLE_BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$INSTALL_PREFIX"
cmake_build "$FIND_EXAMPLE_BUILD"
run cmake -E chdir "$FIND_EXAMPLE_BUILD" "$FIND_EXAMPLE_BUILD/texrender_find_package_example"

rm -rf "$FETCH_EXAMPLE_BUILD"
cmake_configure "$ROOT_DIR/examples/consumer-fetchcontent" "$FETCH_EXAMPLE_BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DTEXRENDER_FETCHCONTENT_SOURCE_DIR="$ROOT_DIR"
cmake_build "$FETCH_EXAMPLE_BUILD"
run cmake -E chdir "$FETCH_EXAMPLE_BUILD" "$FETCH_EXAMPLE_BUILD/texrender_fetchcontent_example"

cmake_configure "$ROOT_DIR" "$RAYLIB_BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DTEXRENDER_BACKEND=raylib
cmake_build "$RAYLIB_BUILD"
run ctest --test-dir "$RAYLIB_BUILD" --output-on-failure

rm -rf "$SDL2_BUILD"
cmake_configure "$ROOT_DIR" "$SDL2_BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DTEXRENDER_BACKEND=sdl2
cmake_build "$SDL2_BUILD"
run ctest --test-dir "$SDL2_BUILD" --output-on-failure

if [[ "${TEXRENDER_SKIP_BACKEND_PACKAGE_SMOKE:-0}" != "1" ]]; then
    run env BUILD_ROOT="${BUILD_ROOT}/backend-packages" "$ROOT_DIR/scripts/verify-backend-packages.sh"
fi

if has doxygen; then
    run rm -rf "$ROOT_DIR/build-api-docs"
    run doxygen "$ROOT_DIR/Doxyfile"
else
    printf '\n==> skipping Doxygen API docs: doxygen is not installed\n'
fi

if has actionlint; then
    run actionlint
else
    printf '\n==> skipping actionlint: not installed\n'
fi

if [[ "${TEXRENDER_SKIP_TSAN_SMOKE:-0}" != "1" ]]; then
    if has clang++; then
        run env BUILD_DIR="${BUILD_ROOT}/tsan" "$ROOT_DIR/scripts/verify-thread-sanitizer.sh"
    else
        printf '\n==> skipping ThreadSanitizer smoke: clang++ is not installed\n'
    fi
fi

if has conan; then
    run conan create "$ROOT_DIR" --build=missing \
        -o '&:backend=headless' \
        -o '&:with_examples=False' \
        -o '&:with_tests=False' \
        -o '&:with_benchmarks=False'
    run conan create "$ROOT_DIR" --build=missing \
        -o '&:backend=svg' \
        -o '&:with_examples=False' \
        -o '&:with_tests=False' \
        -o '&:with_benchmarks=False'
    run conan create "$ROOT_DIR" --build=missing \
        -o '&:backend=sdl2' \
        -o '&:with_examples=False' \
        -o '&:with_tests=False' \
        -o '&:with_benchmarks=False'
    run conan create "$ROOT_DIR" --build=missing \
        -o '&:backend=headless' \
        -o '&:shared=True' \
        -o '&:with_examples=False' \
        -o '&:with_tests=False' \
        -o '&:with_benchmarks=False'
else
    printf '\n==> skipping Conan package validation: conan is not installed\n'
fi

if [[ -n "${VCPKG_ROOT:-}" && -x "${VCPKG_ROOT}/vcpkg" ]]; then
    VCPKG_BUILD="${BUILD_ROOT}/vcpkg"
    rm -rf "$VCPKG_BUILD"
    cmake_configure "$ROOT_DIR" "$VCPKG_BUILD" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" \
        -DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON \
        -DTEXRENDER_BACKEND=headless \
        -DTEXRENDER_BUILD_EXAMPLES=OFF \
        -DTEXRENDER_BUILD_BENCHMARKS=OFF
    cmake_build "$VCPKG_BUILD"
    run ctest --test-dir "$VCPKG_BUILD" --output-on-failure
    run "$ROOT_DIR/scripts/verify-vcpkg-overlay.sh"
    run env TEXRENDER_VCPKG_PACKAGE_SPEC='texrender[sdl2]' "$ROOT_DIR/scripts/verify-vcpkg-overlay.sh"
else
    printf '\n==> skipping vcpkg manifest validation: set VCPKG_ROOT to a vcpkg checkout\n'
fi

if [[ "${TEXRENDER_SKIP_CPACK_SMOKE:-0}" != "1" ]]; then
    run env BUILD_ROOT="${BUILD_ROOT}/cpack-smoke" "$ROOT_DIR/scripts/verify-cpack.sh"
fi

if [[ "${TEXRENDER_SKIP_EXTERNAL_CONSUMER_SMOKE:-0}" != "1" ]]; then
    run env BUILD_ROOT="${BUILD_ROOT}/external-consumers" "$ROOT_DIR/scripts/verify-external-consumers.sh"
fi

if [[ "${TEXRENDER_SKIP_PATH_SMOKE:-0}" != "1" ]]; then
    run env BUILD_ROOT="${BUILD_ROOT}/path smoke" "$ROOT_DIR/scripts/verify-path-with-spaces.sh"
fi

if [[ "${TEXRENDER_SKIP_PKG_CONFIG_SMOKE:-0}" != "1" ]]; then
    if has pkg-config; then
        run env BUILD_ROOT="${BUILD_ROOT}/pkg-config" "$ROOT_DIR/scripts/verify-pkg-config.sh"
    else
        printf '\n==> skipping pkg-config smoke: pkg-config is not installed\n'
    fi
fi

if [[ "${TEXRENDER_SKIP_ARCHIVE_SMOKE:-0}" != "1" ]]; then
    run env BUILD_ROOT="${BUILD_ROOT}/archive-smoke" "$ROOT_DIR/scripts/verify-release-archive.sh"
fi

printf '\nrelease verification completed successfully\n'
