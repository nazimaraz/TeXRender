#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-/tmp/texrender-external-consumers}"
GENERATOR="${GENERATOR:-Ninja}"
SOURCE_REF="${TEXRENDER_ARCHIVE_REF:-HEAD}"
ALLOW_DIRTY_ARCHIVE="${TEXRENDER_ALLOW_DIRTY_ARCHIVE:-0}"
SOURCE_ARCHIVE="${BUILD_ROOT}/source.tar"
SOURCE_DIR="${BUILD_ROOT}/archived-source"
FETCH_PROJECT="${BUILD_ROOT}/fetch-consumer"
FETCH_BUILD="${BUILD_ROOT}/fetch-consumer-build"
INSTALL_BUILD="${BUILD_ROOT}/install-build"
INSTALL_PREFIX="${BUILD_ROOT}/install-prefix"
FIND_PROJECT="${BUILD_ROOT}/find-consumer"
FIND_BUILD="${BUILD_ROOT}/find-consumer-build"

run() {
    printf '\n==> %s\n' "$*"
    "$@"
}

write_fetch_consumer() {
    mkdir -p "$FETCH_PROJECT"
    cat > "$FETCH_PROJECT/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.21)
project(texrender_external_fetch LANGUAGES CXX)
include(FetchContent)
FetchContent_Declare(texrender SOURCE_DIR [[${SOURCE_DIR}]])
set(TEXRENDER_BACKEND headless CACHE STRING "" FORCE)
set(TEXRENDER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TEXRENDER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TEXRENDER_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(texrender)
add_executable(external_fetch main.cpp)
target_link_libraries(external_fetch PRIVATE texrender::texrender)
texrender_copy_resources(external_fetch)
EOF
    cat > "$FETCH_PROJECT/main.cpp" <<'EOF'
#include <memory>
#include <texrender/headless_surface.hpp>
#include <texrender/latex_document.hpp>

#ifndef TEXRENDER_RES_DIR
#error "texrender_copy_resources must define TEXRENDER_RES_DIR"
#endif

int main()
{
    auto surface = std::make_unique<TeXRender::HeadlessSurface>();
    auto* canvas = surface.get();
    TeXRender::LatexDocument document(TEXRENDER_RES_DIR, std::move(surface));
    auto formula = document.render(R"(\sqrt{x+1})", 36.f, 0xff000000);
    canvas->resize(formula.width() + 10, formula.height() + 10);
    canvas->clear(0xffffffff);
    formula.draw(5, 5);
    return canvas->covered_pixels() > 0 ? 0 : 1;
}
EOF
}

write_find_consumer() {
    mkdir -p "$FIND_PROJECT"
    cat > "$FIND_PROJECT/CMakeLists.txt" <<'EOF'
cmake_minimum_required(VERSION 3.21)
project(texrender_external_find LANGUAGES CXX)
find_package(texrender CONFIG REQUIRED)
add_executable(external_find main.cpp)
target_link_libraries(external_find PRIVATE texrender::texrender)
texrender_copy_resources(external_find)
EOF
    cp "$FETCH_PROJECT/main.cpp" "$FIND_PROJECT/main.cpp"
}

rm -rf "$BUILD_ROOT"
mkdir -p "$SOURCE_DIR"
if [[ "$SOURCE_REF" == "HEAD" && "$ALLOW_DIRTY_ARCHIVE" != "1" ]]; then
    if [[ -n "$(git -C "$ROOT_DIR" status --short)" ]]; then
        printf 'working tree has uncommitted changes; %s archives HEAD only and would ignore them\n' "$0" >&2
        printf 'commit/stash the changes, set TEXRENDER_ARCHIVE_REF to a different ref, or rerun with TEXRENDER_ALLOW_DIRTY_ARCHIVE=1.\n' >&2
        exit 1
    fi
elif [[ "$SOURCE_REF" == "HEAD" ]]; then
    printf '\n==> warning: archiving HEAD while ignoring uncommitted working-tree changes\n'
fi
run "$ROOT_DIR/scripts/archive-source-with-submodules.sh" --format tar --ref "$SOURCE_REF" --output "$SOURCE_ARCHIVE"
run tar -xf "$SOURCE_ARCHIVE" -C "$SOURCE_DIR"

write_fetch_consumer
run cmake -S "$FETCH_PROJECT" -B "$FETCH_BUILD" -G "$GENERATOR" -DCMAKE_BUILD_TYPE=Release
run cmake --build "$FETCH_BUILD" -j
run "$FETCH_BUILD/external_fetch"

run cmake -S "$SOURCE_DIR" -B "$INSTALL_BUILD" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DTEXRENDER_BACKEND=headless \
    -DTEXRENDER_BUILD_EXAMPLES=OFF \
    -DTEXRENDER_BUILD_TESTS=OFF \
    -DTEXRENDER_BUILD_BENCHMARKS=OFF
run cmake --build "$INSTALL_BUILD" -j
run cmake --install "$INSTALL_BUILD" --prefix "$INSTALL_PREFIX"

write_find_consumer
run cmake -S "$FIND_PROJECT" -B "$FIND_BUILD" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$INSTALL_PREFIX"
run cmake --build "$FIND_BUILD" -j
run "$FIND_BUILD/external_find"

printf '\nexternal consumer smoke completed successfully\n'
