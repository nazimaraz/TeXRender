# latex-renderer

[![CI](https://github.com/nazimaraz/latex-renderer/actions/workflows/ci.yml/badge.svg)](https://github.com/nazimaraz/latex-renderer/actions/workflows/ci.yml)
[![Package managers](https://github.com/nazimaraz/latex-renderer/actions/workflows/package-managers.yml/badge.svg)](https://github.com/nazimaraz/latex-renderer/actions/workflows/package-managers.yml)
[![Static analysis](https://github.com/nazimaraz/latex-renderer/actions/workflows/static-analysis.yml/badge.svg)](https://github.com/nazimaraz/latex-renderer/actions/workflows/static-analysis.yml)
[![Coverage](https://github.com/nazimaraz/latex-renderer/actions/workflows/coverage.yml/badge.svg)](https://github.com/nazimaraz/latex-renderer/actions/workflows/coverage.yml)
[![Workflow lint](https://github.com/nazimaraz/latex-renderer/actions/workflows/workflow-lint.yml/badge.svg)](https://github.com/nazimaraz/latex-renderer/actions/workflows/workflow-lint.yml)
[![Web demo](https://github.com/nazimaraz/latex-renderer/actions/workflows/web.yml/badge.svg)](https://github.com/nazimaraz/latex-renderer/actions/workflows/web.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![API docs](https://img.shields.io/badge/API-docs-2b6cb0.svg)](https://nazimaraz.github.io/latex-renderer/api/)

Render LaTeX math **directly into a GUI window** from C++.

**[▶ Try the live editor in your browser](https://nazimaraz.github.io/latex-renderer/)** —
the same C++ engine compiled to WebAssembly.

[MicroTeX](https://github.com/NanoMichael/MicroTeX) does the hard part — parsing
LaTeX and laying out the boxes — but ships only Cairo / GDI+ / Qt / Skia
backends. Tools like [latexpp](https://github.com/goldsborough/latexpp) only
render to image files. **latex-renderer** drives MicroTeX through a small,
pluggable rendering layer so a formula can be drawn live into a window.

The GUI backend is [raylib](https://www.raylib.com/); a second, dependency-free
**headless** surface rasterises into an in-memory RGBA buffer and ships in every
build — inject it into a document to [render offscreen](#offscreen-rendering-no-window)
(it also powers the GPU-less test suite). The `IDrawSurface` seam is proven by two
implementations; adding SDL2/SFML/etc. means implementing one small interface —
nothing in the core changes.

![demo](docs/demo.png)

## Why use latex-renderer?

* **No temporary images or subprocesses.** Parse a formula once, keep a
  `RenderHandle`, and draw it inside your normal frame loop.
* **Windowed and windowless output.** The same API supports raylib overlays,
  server-style headless rendering, tests, and WebAssembly.
* **Dependency-shaped CMake.** Consumers can use `FetchContent`,
  `add_subdirectory`, installed `find_package`, vcpkg manifests, or Conan.
* **Small backend contract.** Custom engines only implement `IDrawSurface`; the
  parser, transforms, FreeType glyph rasterization, and caches stay shared.

See the [gallery](docs/gallery.md), [integration examples](docs/integration-examples.md),
hosted [API reference](https://nazimaraz.github.io/latex-renderer/api/), and
[case study](docs/case-study.md).
The [roadmap](docs/roadmap.md) lists planned work and non-goals.

## How it works

```
                 ┌──────────────────────────────────────────┐
   LaTeX string  │  LatexDocument  ──▶  MicroTeX core        │  parse + layout
                 │   │   (owns surface) (FetchContent)        │
                 │   └─▶ RenderHandle.draw(x, y)              │
                 │            │                               │
                 │  Graphics2DAdapter : tex::Graphics2D       │  transforms, color,
                 │       │   (FreeType glyph rasterisation)   │  glyph cache
                 │       ▼                                    │
                 │  IDrawSurface  ◀── RaylibSurface           │  paint primitives
                 └──────────────────────────────────────────┘
        you own the raylib window + render loop (InitWindow, BeginDrawing, ...)
```

You drive raylib yourself — `InitWindow`, `BeginDrawing`/`EndDrawing`,
`WindowShouldClose`, `CloseWindow`. The library never wraps the window or the
loop; it just paints a parsed formula into the current frame through a
`RaylibSurface`.

For a deeper design walk-through, see [`docs/architecture.md`](docs/architecture.md).
For public API and lifetime details, see [`docs/api.md`](docs/api.md).

* **`tex::Graphics2D` is character-based** (`drawChar(wchar_t, x, y)`), drawing by
  Unicode codepoint through each TeX font's cmap. So the integration rasterises
  glyphs from the bundled TeX TrueType fonts with **FreeType** (in `GlyphCache`),
  once, behind a backend-neutral seam.
* **`IDrawSurface`** is the only interface a GUI backend implements — a handful of
  paint primitives in screen pixels. The affine transform, color and glyph
  caching all live in the shared `Graphics2DAdapter`. `RaylibSurface` is the
  raylib implementation.

## Build

Requirements: a C++23 toolchain with `std::format`/`std::print` support — **GCC
≥ 14, Clang ≥ 18 (libc++), MSVC ≥ 19.40, or a recent Apple Clang** — and CMake ≥
3.21. That toolchain compiles texgui *itself*; consuming projects only need
**C++17** (the standard texgui's public headers require and propagate). tinyxml2,
FreeType and raylib are resolved with `find_package`, falling back to
`FetchContent`; MicroTeX's core is always fetched and compiled from source. On
macOS `brew install freetype raylib tinyxml2` makes configuration instant.

```sh
cmake -S . -B build
cmake --build build -j
./build/texgui_minimal      # draws a single formula in a window
./build/texgui_demo         # cycle samples (SPACE/arrows), resize text (UP/DOWN)
./build/texgui_offscreen    # render to formula.png — no window needed
ctest --test-dir build      # run the unit tests
```

The smallest possible program is [`examples/minimal.cpp`](examples/minimal.cpp):
parse one formula, draw it in a loop.

More copyable consumer examples live in
[`docs/integration-examples.md`](docs/integration-examples.md).

Common contributor builds are also available as CMake presets:

```sh
cmake --preset headless-debug
cmake --build --preset headless-debug
ctest --preset headless-debug
```

See [`docs/build-options.md`](docs/build-options.md) for presets, shared-library
builds, and the `TEXGUI_WARNINGS_AS_ERRORS` policy.

## Using texgui in your project

### FetchContent / add_subdirectory

texgui can build its dependencies (MicroTeX, FreeType, tinyxml2, raylib) via
CMake `FetchContent`, so the simplest way to consume it is to fetch the project
the same way — no system packages required:

```cmake
include(FetchContent)
FetchContent_Declare(
    texgui
    GIT_REPOSITORY https://github.com/nazimaraz/latex-renderer.git
    GIT_TAG        v0.1.0        # pin a tag or commit for reproducible builds
)
FetchContent_MakeAvailable(texgui)

target_link_libraries(my_app PRIVATE texgui::texgui)
texgui_copy_resources(my_app)
```

Or, if you vendor the source, `add_subdirectory(path/to/latex-renderer)` and link
`texgui::texgui` the same way. Either path skips texgui's own examples and tests
automatically when it is built as a subproject. You still drive raylib yourself
(`InitWindow`, the render loop, `CloseWindow`) — see [Usage](#usage).

### find_package install

You can also install and consume the exported CMake package:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTEXGUI_BACKEND=headless
cmake --build build -j
cmake --install build --prefix /path/to/prefix
```

```cmake
find_package(texgui CONFIG REQUIRED)

target_link_libraries(my_app PRIVATE texgui::texgui)
texgui_copy_resources(my_app)
```

The installed package uses `find_dependency` for FreeType, tinyxml2 and, when
installed with `TEXGUI_BACKEND=raylib`, raylib. Raylib package export is enabled
only when raylib was found with `find_package(raylib)`; if raylib is fetched from
source, prefer `FetchContent` / `add_subdirectory`, or install the headless
backend as shown above.

Starter metadata is included for package-manager workflows: [`vcpkg.json`](vcpkg.json)
and [`conanfile.py`](conanfile.py). They are intended as project-local manifests
until the project is submitted to public registries.

### Shipping the resource directory

MicroTeX needs its `res/` directory (fonts + metrics) **at runtime**; you pass its
path to `LatexDocument`. The CMake helper `texgui_copy_resources(my_app)` copies
that directory to a target-specific folder next to your executable and defines
`TEXGUI_RES_DIR` for that target:

```cmake
target_link_libraries(my_app PRIVATE texgui::texgui)
texgui_copy_resources(my_app)
```

```cpp
TexGui::LatexDocument document(TEXGUI_RES_DIR);
```

If you need custom packaging, copy `TEXGUI_RESOURCE_DIR` yourself and define your
own resource path macro or configuration value.

## Usage

Pure raylib — you own the window and the loop:

```cpp
#include <raylib.h>
#include <texgui/latex_document.hpp>

InitWindow(640, 360, "LaTeX");

// Scope the document/handle so they are destroyed (freeing GPU glyph textures)
// while the window is still alive — see the lifetime note below.
{
    TexGui::LatexDocument document("/path/to/MicroTeX/res");
    auto formula = document.render(R"(\sqrt{\frac{a^2+b^2}{c}})", /*size*/ 50.f, /*argb*/ 0xff000000);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        formula.draw(40, 40);  // top-left at (40, 40), into the current frame
        EndDrawing();
    }
}

CloseWindow();
```

`document.render(...)` returns a move-only `RenderHandle` exposing `width()`,
`height()`, `depth()`, `baseline()` and `set_foreground()`. A `RenderHandle`
shares ownership of its document's runtime state, so it stays valid to draw and to
destroy even if the `LatexDocument` is released first.

The optional fourth argument, `wrap_width`, wraps the formula to that many pixels;
the default (`0`) lays it out on a single unbounded line.

**Malformed LaTeX throws.** `render()` throws a `std::exception`-derived error
(MicroTeX's `tex::ex_parse`) when the input doesn't parse — fine for the trusted,
literal formulas above, but for caller-supplied input use the non-throwing
`try_render()`, which returns `std::optional<RenderHandle>`:

```cpp
if (auto formula = document.try_render(user_input, 50.f, 0xff000000))
    formula->draw(40, 40);   // std::nullopt instead of an exception on a parse error
```

The simple `try_render()` overload applies untrusted-input guardrails before
calling MicroTeX: 64 KiB maximum input, text size up to 512 px, and wrap width up
to 32768 px. Tune those limits with `RenderOptions`:

```cpp
TexGui::RenderOptions options;
options.text_size = 50.f;
options.argb = 0xff000000;
options.max_input_bytes = 256u * 1024u;

if (auto formula = document.try_render(user_input, options))
    formula->draw(40, 40);
```

If the application needs to show a useful validation message, use
`try_render_detailed()` instead. It preserves a `RenderErrorCode` plus a
message while keeping the same `noexcept` boundary:

```cpp
auto result = document.try_render_detailed(user_input, options);
if (result)
{
    result.value().draw(40, 40);
}
else
{
    show_error(result.error().message);
}
```

See [`examples/live_edit.cpp`](examples/live_edit.cpp) for a live editor built on
`try_render`.

**Destroy documents and handles before `CloseWindow()`.** The backend frees its GPU
glyph textures when the last `RenderHandle`/`LatexDocument` is destroyed, which
needs the window still alive; tear them down (e.g. by scoping them) before
`CloseWindow()`.

### Offscreen rendering (no window)

A document draws into the surface it owns — by default the built backend's
surface, but you can inject any `IDrawSurface` instead. Inject the dependency-free
`HeadlessSurface` (available in every build, including raylib ones) to rasterise
into an in-memory RGBA canvas and read the pixels back:

```cpp
#include <texgui/headless_surface.hpp>
#include <texgui/latex_document.hpp>

auto owned = std::make_unique<TexGui::HeadlessSurface>();
auto& canvas = *owned;                          // keep a handle; the document takes ownership
TexGui::LatexDocument document("/path/to/MicroTeX/res", std::move(owned));

auto formula = document.render(R"(\int_0^1 x^2\,dx)", 48.f, 0xff000000);
canvas.resize(formula.width() + 20, formula.height() + 20);
canvas.clear(0xffffffff);                       // opaque white
formula.draw(10, 10);

canvas.write_png("formula.png");                // PNG, PPM, or raw RGBA8 pixels
```

`LatexDocument::surface()` returns the surface a document draws into (the
injected one, or the built backend's default). A runnable version of the above
is [`examples/offscreen.cpp`](examples/offscreen.cpp) — the one example that
builds under every backend, including `TEXGUI_BACKEND=headless`.

## Threading and multiple documents

* **Render on one thread.** A process-wide glyph cache — and the GPU textures
  behind it — is shared by every formula, with no internal locking (raylib drawing
  must happen on a single thread regardless). Parse, lay out and draw from one
  thread. Constructing and destroying `LatexDocument`s *is* internally serialised.
* **One resource root per process.** MicroTeX initialises from a single global
  resource directory the first time a `LatexDocument` is created, and stays
  initialised for the lifetime of the process (it cannot be safely re-initialised);
  a later document built from a *different* `res_root` reuses the first. Use the
  same `res_root` everywhere. Creating and destroying documents repeatedly is safe.

Set `TEXGUI_DEBUG=1` to print diagnostics — failed font loads and resource-root
conflicts — to stderr; output is silent otherwise.

Known constraints and tradeoffs are collected in
[`docs/known-limitations.md`](docs/known-limitations.md).

## Adding a backend

Implement `IDrawSurface` from
[`include/texgui/draw_surface.hpp`](include/texgui/draw_surface.hpp) — a handful
of paint primitives plus `draw_glyph` — and inject it into the document. **No
fork required:**

```cpp
TexGui::LatexDocument document(res_root, std::make_unique<MySdlSurface>());
```

The compact [`HeadlessSurface`](source/backend/headless/headless_surface.cpp) is a
complete, dependency-free reference implementation to copy from. You keep using
your toolkit's own windowing; only the paint primitives are abstracted. The core,
`Graphics2DAdapter`, and the FreeType glyph cache are reused unchanged.

To ship a backend in-tree instead (selected by `TEXGUI_BACKEND` as the default
surface), add its sources plus a small `make_surface` factory file under
`source/backend/<name>/` and a branch in `CMakeLists.txt`, the way the raylib
backend does.

## WebAssembly

The raylib backend compiles to WebAssembly, so the live editor runs in the
browser: **<https://nazimaraz.github.io/latex-renderer/>**. With the
[Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
activated:

```sh
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web -j
python3 -m http.server -d build-web/site    # → http://localhost:8000
```

The build lands in `build-web/site/` as a self-contained static page —
`index.html` + `.js`/`.wasm`, plus a ~2 MB `.data` pack holding MicroTeX's
fonts and metrics, mounted at `/res` in the virtual filesystem. The `web`
workflow rebuilds it on every push to `main` and publishes it to GitHub Pages.

Two wasm-specific notes, both handled by the build: native wasm exceptions are
enabled (`-fwasm-exceptions`) because MicroTeX reports parse errors by
throwing, and [`examples/live_edit.cpp`](examples/live_edit.cpp) shows the
standard raylib pattern for driving the same per-frame function from a native
while-loop or the browser's requestAnimationFrame
(`emscripten_set_main_loop`).

## Tests

```sh
cmake -S . -B build -DTEXGUI_BACKEND=headless   # no GPU/display needed
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The suite — transform math, the FreeType glyph cache against the bundled fonts,
input-limit/error-diagnostic checks, an end-to-end formula render into an
injected headless canvas, PNG writing, handle lifetime stress, C++17 public
header compatibility, structural visual regression, and a separate CMake
consumer smoke test — needs no window or GPU and runs under either backend;
`TEXGUI_BACKEND=headless` additionally drops the raylib dependency entirely. CI
exercises this on Linux, macOS and Windows, the raylib backend + examples on
Linux and macOS, runs the suite under ASan + UBSan, checks vcpkg/Conan
packaging, verifies workflow linting and shared-library builds, uploads
visual-regression artifacts on failure, gates formatting with clang-format, runs
clang-tidy, and publishes coverage artifacts.

Additional developer tools:

```sh
cmake --build build --target texgui_render_benchmark -j
./build/texgui_render_benchmark
```

Fuzzing instructions live in [`docs/fuzzing.md`](docs/fuzzing.md); benchmark
notes live in [`docs/benchmarks.md`](docs/benchmarks.md), with reference numbers
in [`docs/performance-baseline.md`](docs/performance-baseline.md). Static
analysis and coverage workflows are documented in
[`docs/static-analysis.md`](docs/static-analysis.md) and
[`docs/coverage.md`](docs/coverage.md). Workflow linting is documented in
[`docs/workflow-linting.md`](docs/workflow-linting.md).

## License

latex-renderer is released under the [MIT License](LICENSE). It builds on
MicroTeX, raylib, FreeType, tinyxml2 and the bundled TeX fonts, each under its own
(permissive) license — see [THIRD_PARTY.md](THIRD_PARTY.md) before redistributing
a binary.
