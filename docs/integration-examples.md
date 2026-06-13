# Integration Examples

These examples are small on purpose: each one shows one dependency story without
extra application structure.

## FetchContent

Use this when you want a consuming CMake project to fetch texgui directly:

```sh
cmake -S examples/consumer-fetchcontent -B build-fetch-example \
  -DTEXGUI_FETCHCONTENT_SOURCE_DIR="$PWD" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-fetch-example -j
./build-fetch-example/texgui_fetchcontent_example
```

The example lives in [`examples/consumer-fetchcontent`](../examples/consumer-fetchcontent).
It pins `TEXGUI_BACKEND=headless` and writes `fetchcontent-example.png`.

## Installed `find_package`

Use this when texgui has already been installed into a prefix:

```sh
cmake -S . -B build-headless -DTEXGUI_BACKEND=headless -DCMAKE_BUILD_TYPE=Release
cmake --build build-headless -j
cmake --install build-headless --prefix /tmp/texgui-prefix

cmake -S examples/consumer-find-package -B build-find-example \
  -DCMAKE_PREFIX_PATH=/tmp/texgui-prefix \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-find-example -j
./build-find-example/texgui_find_package_example
```

The example lives in [`examples/consumer-find-package`](../examples/consumer-find-package).
It uses `find_package(texgui CONFIG REQUIRED)`, links `texgui::texgui`, then calls
`texgui_copy_resources()`.

## Installed `pkg-config`

When texgui is installed, it also writes `texgui.pc` for non-CMake consumers.
Use `--static` when linking the default static package so FreeType and tinyxml2
are included in the link flags:

```sh
cmake -S . -B build-headless \
  -DTEXGUI_BACKEND=headless \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/tmp/texgui-prefix
cmake --build build-headless -j
cmake --install build-headless

export PKG_CONFIG_PATH=/tmp/texgui-prefix/lib/pkgconfig:$PKG_CONFIG_PATH
c++ -std=c++17 my_formula_tool.cpp \
  -DTEXGUI_RES_DIR=\"$(pkg-config --variable=resdir texgui)\" \
  $(pkg-config --cflags --libs --static texgui) \
  -o my_formula_tool
```

The `resdir` variable points at the installed MicroTeX resource directory for
simple command-line consumers.

## Headless Batch Rendering

[`examples/headless_batch.cpp`](../examples/headless_batch.cpp) renders one or
more formulas to PNG files without creating a window:

```sh
cmake -S . -B build-headless -DTEXGUI_BACKEND=headless -DCMAKE_BUILD_TYPE=Release
cmake --build build-headless --target texgui_headless_batch -j
./build-headless/texgui_headless_batch 'e^{i\pi}+1=0' '\int_0^1 x^2\,dx'
```

## SVG Export

[`examples/svg_export.cpp`](../examples/svg_export.cpp) renders one formula to a
self-contained SVG:

```sh
cmake -S . -B build-svg -DTEXGUI_BACKEND=svg -DCMAKE_BUILD_TYPE=Release
cmake --build build-svg --target texgui_svg_export -j
./build-svg/texgui_svg_export '\sqrt{x+1}' formula.svg
```

You can also inject `TexGui::SvgSurface` from any backend build, the same way
the offscreen examples inject `HeadlessSurface`.

This is the shape to use for server-side preview generation, CI image snapshots,
or command-line tools.

## Custom Surface

[`examples/custom_surface.cpp`](../examples/custom_surface.cpp) implements a
tiny `IDrawSurface` that counts draw calls. It is the shortest example of
integrating texgui with a renderer or command buffer that is not built into this
repository:

```sh
cmake -S . -B build-headless -DTEXGUI_BACKEND=headless -DCMAKE_BUILD_TYPE=Release
cmake --build build-headless --target texgui_custom_surface -j
./build-headless/texgui_custom_surface
```

Real backends translate the same methods to their graphics API while texgui keeps
ownership of parsing, layout, transforms, and glyph rasterization.

## Raylib Overlay

[`examples/raylib_overlay.cpp`](../examples/raylib_overlay.cpp) shows texgui as
a regular layer inside a raylib scene:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target texgui_raylib_overlay -j
./build/texgui_raylib_overlay
```

The example still owns the raylib window and frame loop. texgui only draws the
formula into the current frame.

## SDL2 Minimal

[`examples/sdl2_minimal.cpp`](../examples/sdl2_minimal.cpp) shows the same model
inside an `SDL_Renderer`:

```sh
cmake -S . -B build-sdl2 -DTEXGUI_BACKEND=sdl2 -DCMAKE_BUILD_TYPE=Release
cmake --build build-sdl2 --target texgui_sdl2_minimal -j
./build-sdl2/texgui_sdl2_minimal
```

Register the renderer with `TexGui::set_default_sdl_renderer(renderer)` before
constructing a default `LatexDocument`, or inject `TexGui::Sdl2Surface`
explicitly.
