# Integration Examples

These examples are small on purpose: each one shows one dependency story without
extra application structure.

For application-shaped snippets such as HUD formulas, PNG generation, SVG export
pipelines, custom renderer adapters, and safe user-input handling, see
[`docs/recipes.md`](recipes.md).

## FetchContent

Use this when you want a consuming CMake project to fetch texrender directly:

```sh
cmake -S examples/consumer-fetchcontent -B build-fetch-example \
  -DTEXRENDER_FETCHCONTENT_SOURCE_DIR="$PWD" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-fetch-example -j
./build-fetch-example/texrender_fetchcontent_example
```

The example lives in [`examples/consumer-fetchcontent`](../examples/consumer-fetchcontent).
It pins `TEXRENDER_BACKEND=headless` and writes `fetchcontent-example.png`.

## Installed `find_package`

Use this when texrender has already been installed into a prefix:

```sh
cmake -S . -B build-headless -DTEXRENDER_BACKEND=headless -DCMAKE_BUILD_TYPE=Release
cmake --build build-headless -j
cmake --install build-headless --prefix /tmp/texrender-prefix

cmake -S examples/consumer-find-package -B build-find-example \
  -DCMAKE_PREFIX_PATH=/tmp/texrender-prefix \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-find-example -j
./build-find-example/texrender_find_package_example
```

The example lives in [`examples/consumer-find-package`](../examples/consumer-find-package).
It uses `find_package(texrender CONFIG REQUIRED)`, links `texrender::texrender`, then calls
`texrender_copy_resources()`.

## Installed `pkg-config`

When texrender is installed, it also writes `texrender.pc` for non-CMake consumers.
Use `--static` when linking the default static package so FreeType and tinyxml2
are included in the link flags. A complete Makefile example lives in
[`examples/consumer-pkg-config`](../examples/consumer-pkg-config):

```sh
cmake -S . -B build-headless \
  -DTEXRENDER_BACKEND=headless \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/tmp/texrender-prefix
cmake --build build-headless -j
cmake --install build-headless

export PKG_CONFIG_PATH=/tmp/texrender-prefix/lib/pkgconfig:$PKG_CONFIG_PATH
c++ -std=c++17 my_formula_tool.cpp \
  -DTEXRENDER_RES_DIR=\"$(pkg-config --variable=resdir texrender)\" \
  $(pkg-config --cflags --libs --static texrender) \
  -o my_formula_tool
make -C examples/consumer-pkg-config run
```

The `resdir` variable points at the installed MicroTeX resource directory for
simple command-line consumers.

## Headless Batch Rendering

[`examples/headless_batch.cpp`](../examples/headless_batch.cpp) renders one or
more formulas to PNG files without creating a window:

```sh
cmake -S . -B build-headless -DTEXRENDER_BACKEND=headless -DCMAKE_BUILD_TYPE=Release
cmake --build build-headless --target texrender_headless_batch -j
./build-headless/texrender_headless_batch 'e^{i\pi}+1=0' '\int_0^1 x^2\,dx'
```

## SVG Export

[`examples/svg_export.cpp`](../examples/svg_export.cpp) renders one formula to a
self-contained SVG:

```sh
cmake -S . -B build-svg -DTEXRENDER_BACKEND=svg -DCMAKE_BUILD_TYPE=Release
cmake --build build-svg --target texrender_svg_export -j
./build-svg/texrender_svg_export '\sqrt{x+1}' formula.svg
```

You can also inject `TeXRender::SvgSurface` from any backend build, the same way
the offscreen examples inject `HeadlessSurface`.

This is the shape to use for server-side preview generation, CI image snapshots,
or command-line tools.

## Custom Surface

[`examples/custom_surface.cpp`](../examples/custom_surface.cpp) implements a
tiny `IDrawSurface` that counts draw calls. It is the shortest example of
integrating texrender with a renderer or command buffer that is not built into this
repository:

```sh
cmake -S . -B build-headless -DTEXRENDER_BACKEND=headless -DCMAKE_BUILD_TYPE=Release
cmake --build build-headless --target texrender_custom_surface -j
./build-headless/texrender_custom_surface
```

Real backends translate the same methods to their graphics API while texrender keeps
ownership of parsing, layout, transforms, and glyph rasterization.

## Formula Label Widget

[`examples/formula_label_widget.cpp`](../examples/formula_label_widget.cpp) shows
TeXRender wrapped inside a small reusable application object. It keeps a parsed
`RenderHandle`, exposes layout metrics through `FormulaBox`, and draws the label
baseline-aligned into an injected surface:

```sh
cmake -S . -B build-headless -DTEXRENDER_BACKEND=headless -DCMAKE_BUILD_TYPE=Release
cmake --build build-headless --target texrender_formula_label_widget -j
./build-headless/texrender_formula_label_widget
```

The example writes `formula-label-widget.png` and is intentionally shaped like a
dependency being embedded into a larger UI layer.

## Raylib Overlay

[`examples/raylib_overlay.cpp`](../examples/raylib_overlay.cpp) shows texrender as
a regular layer inside a raylib scene:

```sh
cmake -S . -B build-raylib -DTEXRENDER_BACKEND=raylib -DCMAKE_BUILD_TYPE=Release
cmake --build build-raylib --target texrender_raylib_overlay -j
./build-raylib/texrender_raylib_overlay
```

The example still owns the raylib window and frame loop. TeXRender only draws the
formula into the current frame.

## SDL2 Minimal

[`examples/sdl2_minimal.cpp`](../examples/sdl2_minimal.cpp) shows the same model
inside an `SDL_Renderer`:

```sh
cmake -S . -B build-sdl2 -DTEXRENDER_BACKEND=sdl2 -DCMAKE_BUILD_TYPE=Release
cmake --build build-sdl2 --target texrender_sdl2_minimal -j
./build-sdl2/texrender_sdl2_minimal
```

Register the renderer with `TeXRender::set_default_sdl_renderer(renderer)` before
constructing a default `LatexDocument`, or inject `TeXRender::Sdl2Surface`
explicitly.
