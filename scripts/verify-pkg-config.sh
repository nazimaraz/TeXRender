#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-/tmp/texgui-pkg-config}"
GENERATOR="${GENERATOR:-Ninja}"
BUILD_DIR="${BUILD_ROOT}/build"
INSTALL_PREFIX="${BUILD_ROOT}/prefix"
CONSUMER_DIR="${BUILD_ROOT}/consumer"
CXX_BIN="${CXX:-c++}"

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

if ! has pkg-config; then
    printf 'pkg-config is required\n' >&2
    exit 1
fi

if ! has "$CXX_BIN"; then
    printf 'C++ compiler is required: %s\n' "$CXX_BIN" >&2
    exit 1
fi

rm -rf "$BUILD_ROOT"
mkdir -p "$CONSUMER_DIR"

run cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    -DTEXGUI_BACKEND=headless \
    -DTEXGUI_BUILD_EXAMPLES=OFF \
    -DTEXGUI_BUILD_TESTS=OFF \
    -DTEXGUI_BUILD_BENCHMARKS=OFF
run cmake --build "$BUILD_DIR" -j
run cmake --install "$BUILD_DIR"

PC_FILE="$(find "$INSTALL_PREFIX" -path '*/pkgconfig/texgui.pc' -type f -print -quit)"
if [[ -z "$PC_FILE" ]]; then
    printf 'installed texgui.pc was not found under %s\n' "$INSTALL_PREFIX" >&2
    exit 1
fi

pkg_config_path="$(dirname "$PC_FILE")"
for dir in \
    /opt/homebrew/lib/pkgconfig \
    /usr/local/lib/pkgconfig \
    /opt/local/lib/pkgconfig \
    /opt/homebrew/Library/Homebrew/os/mac/pkgconfig/* \
    /usr/local/Homebrew/Library/Homebrew/os/mac/pkgconfig/*; do
    if [[ -d "$dir" ]]; then
        pkg_config_path="${pkg_config_path}:$dir"
    fi
done
if has brew; then
    for formula in freetype tinyxml2 raylib sdl2 zlib bzip2 libpng; do
        formula_prefix="$(brew --prefix "$formula" 2>/dev/null || true)"
        if [[ -n "$formula_prefix" && -d "$formula_prefix/lib/pkgconfig" ]]; then
            pkg_config_path="${pkg_config_path}:$formula_prefix/lib/pkgconfig"
        fi
    done
fi
export PKG_CONFIG_PATH="${pkg_config_path}${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
run pkg-config --exists texgui
run pkg-config --print-errors --exists --static texgui

cat > "$CONSUMER_DIR/main.cpp" <<'EOF'
#include <memory>
#include <texgui/headless_surface.hpp>
#include <texgui/latex_document.hpp>

#ifndef TEXGUI_RES_DIR
#error "define TEXGUI_RES_DIR from pkg-config --variable=resdir texgui"
#endif

int main()
{
    auto surface = std::make_unique<TexGui::HeadlessSurface>();
    auto* const canvas = surface.get();
    TexGui::LatexDocument document(TEXGUI_RES_DIR, std::move(surface));
    const auto formula = document.render(R"(\sqrt{x+1})", 36.f, 0xff000000);
    canvas->resize(formula.width() + 10, formula.height() + 10);
    canvas->clear(0xffffffff);
    formula.draw(5, 5);
    return canvas->covered_pixels() > 0 ? 0 : 1;
}
EOF

res_dir="$(pkg-config --variable=resdir texgui)"
# pkg-config outputs compiler/linker fragments intentionally consumed by the
# compiler command below. The smoke path avoids spaces so shell splitting is OK.
run "$CXX_BIN" -std=c++17 "$CONSUMER_DIR/main.cpp" "-DTEXGUI_RES_DIR=\"$res_dir\"" $(pkg-config --cflags --libs --static texgui) \
    -o "$CONSUMER_DIR/pkg-config-consumer"
run "$CONSUMER_DIR/pkg-config-consumer"

printf '\npkg-config consumer smoke completed successfully\n'
