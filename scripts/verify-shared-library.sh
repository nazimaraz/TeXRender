#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-/tmp/texrender-shared-library}"
GENERATOR="${GENERATOR:-Ninja}"
BACKEND="${TEXRENDER_SHARED_BACKEND:-headless}"
BUILD_DIR="${BUILD_ROOT}/build-${BACKEND}"
INSTALL_PREFIX="${BUILD_ROOT}/prefix-${BACKEND}"
PACKAGE_CONSUMER_BUILD="${BUILD_ROOT}/package-consumer-${BACKEND}"
FIND_EXAMPLE_BUILD="${BUILD_ROOT}/find-example-${BACKEND}"
PROJECT_VERSION="$(sed -nE 's/^[[:space:]]*VERSION[[:space:]]+([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' "$ROOT_DIR/CMakeLists.txt" | head -n 1)"
PROJECT_SOVERSION="${PROJECT_VERSION%%.*}"

if [[ -z "$PROJECT_VERSION" || -z "$PROJECT_SOVERSION" ]]; then
    printf 'failed to parse project version from %s\n' "$ROOT_DIR/CMakeLists.txt" >&2
    exit 1
fi

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

shared_library_path() {
    find "$INSTALL_PREFIX" \( -type f -o -type l \) \( \
        -name 'libtexrender.so' -o \
        -name 'libtexrender.so.*' -o \
        -name 'libtexrender*.dylib' -o \
        -name 'texrender.dll' \
        \) | sort | head -n 1
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
    if [[ -n "$(shared_library_path)" ]]; then
        return
    fi

    printf 'expected an installed shared texrender library under %s\n' "$INSTALL_PREFIX" >&2
    find "$INSTALL_PREFIX" -maxdepth 3 -type f >&2 || true
    exit 1
}

require_versioned_shared_artifacts() {
    case "$(uname -s)" in
        Darwin)
            for artifact in \
                "$INSTALL_PREFIX/lib/libtexrender.dylib" \
                "$INSTALL_PREFIX/lib/libtexrender.${PROJECT_SOVERSION}.dylib" \
                "$INSTALL_PREFIX/lib/libtexrender.${PROJECT_VERSION}.dylib"
            do
                if [[ ! -e "$artifact" ]]; then
                    printf 'expected installed shared-library artifact: %s\n' "$artifact" >&2
                    find "$INSTALL_PREFIX/lib" -maxdepth 1 -name 'libtexrender*.dylib' -print >&2 || true
                    exit 1
                fi
            done
            ;;
        MINGW* | MSYS* | CYGWIN*)
            # CMake does not encode SOVERSION into the Windows DLL filename.
            ;;
        *)
            for artifact in \
                "$INSTALL_PREFIX/lib/libtexrender.so" \
                "$INSTALL_PREFIX/lib/libtexrender.so.${PROJECT_SOVERSION}" \
                "$INSTALL_PREFIX/lib/libtexrender.so.${PROJECT_VERSION}"
            do
                if [[ ! -e "$artifact" ]]; then
                    printf 'expected installed shared-library artifact: %s\n' "$artifact" >&2
                    find "$INSTALL_PREFIX/lib" -maxdepth 1 -name 'libtexrender.so*' -print >&2 || true
                    exit 1
                fi
            done
            ;;
    esac

    printf '\nshared-library version artifacts checked\n'
    printf 'version: %s\n' "$PROJECT_VERSION"
    printf 'soversion: %s\n' "$PROJECT_SOVERSION"
}

defined_global_symbols() {
    local library="$1"
    case "$(uname -s)" in
        Darwin)
            nm -gU "$library"
            ;;
        MINGW* | MSYS* | CYGWIN*)
            nm -g --defined-only "$library"
            ;;
        *)
            nm -D --defined-only "$library"
            ;;
    esac
}

demangle_symbols() {
    if has c++filt; then
        c++filt
    else
        cat
    fi
}

audit_exported_symbols() {
    if ! has nm; then
        printf 'nm is required to audit shared-library exports\n' >&2
        exit 1
    fi

    local library
    library="$(shared_library_path)"
    if [[ -z "$library" ]]; then
        printf 'expected shared library before export audit\n' >&2
        exit 1
    fi

    local symbols_file="${BUILD_ROOT}/exported-symbols-${BACKEND}.txt"
    defined_global_symbols "$library" | demangle_symbols > "$symbols_file"

    local required_symbol
    for required_symbol in \
        'TeXRender::LatexDocument::render' \
        'TeXRender::RenderHandle::draw' \
        'TeXRender::HeadlessSurface::draw_glyph' \
        'TeXRender::SvgSurface::svg' \
        'TeXRender::cache_limits' \
        'TeXRender::shutdown'
    do
        if ! grep -Fq "$required_symbol" "$symbols_file"; then
            printf 'expected exported symbol matching "%s" in %s\n' "$required_symbol" "$library" >&2
            cat "$symbols_file" >&2
            exit 1
        fi
    done

    local forbidden_pattern
    for forbidden_pattern in \
        'TeXRender::detail::' \
        'tex::' \
        'TeXRender::HeadlessSurface::blend' \
        'TeXRender::SvgSurface::paint_attributes' \
        'microtex'
    do
        if grep -Fq "$forbidden_pattern" "$symbols_file"; then
            printf 'unexpected private/internal export matching "%s" in %s\n' "$forbidden_pattern" "$library" >&2
            grep -F "$forbidden_pattern" "$symbols_file" >&2
            exit 1
        fi
    done

    printf '\nexported-symbol audit passed\n'
    printf 'library: %s\n' "$library"
    printf 'symbols: %s\n' "$(wc -l < "$symbols_file" | tr -d '[:space:]')"
}

case "$BACKEND" in
    headless | svg)
        ;;
    *)
        printf "unsupported TEXRENDER_SHARED_BACKEND='%s'; use headless or svg for dependency-free shared smoke\n" "$BACKEND" >&2
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
    -DTEXRENDER_BACKEND="$BACKEND" \
    -DTEXRENDER_INSTALL=ON \
    -DTEXRENDER_BUILD_EXAMPLES=OFF \
    -DTEXRENDER_BUILD_BENCHMARKS=OFF
run cmake --build "$BUILD_DIR" -j
run_with_shared_runtime ctest --test-dir "$BUILD_DIR" --output-on-failure
run cmake --install "$BUILD_DIR" --prefix "$INSTALL_PREFIX"
require_shared_library
require_versioned_shared_artifacts
audit_exported_symbols

run env \
    BUILD_ROOT="${BUILD_ROOT}/installed-header-smoke-${BACKEND}" \
    TEXRENDER_HEADER_USE_EXISTING_INSTALL=1 \
    TEXRENDER_HEADER_BACKEND="$BACKEND" \
    TEXRENDER_HEADER_INSTALL_PREFIX="$INSTALL_PREFIX" \
    TEXRENDER_HEADER_CONSUMER_BUILD="${BUILD_ROOT}/installed-headers-${BACKEND}" \
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
run_with_shared_runtime cmake -E chdir "$FIND_EXAMPLE_BUILD" "$FIND_EXAMPLE_BUILD/texrender_find_package_example"

printf '\nshared-library package smoke completed successfully\n'
printf 'backend: %s\n' "$BACKEND"
printf 'prefix: %s\n' "$INSTALL_PREFIX"
