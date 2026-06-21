# TeXRender

[![CI](https://github.com/nazimaraz/TeXRender/actions/workflows/ci.yml/badge.svg)](https://github.com/nazimaraz/TeXRender/actions/workflows/ci.yml)
[![Package managers](https://github.com/nazimaraz/TeXRender/actions/workflows/package-managers.yml/badge.svg)](https://github.com/nazimaraz/TeXRender/actions/workflows/package-managers.yml)
[![Registry readiness](https://github.com/nazimaraz/TeXRender/actions/workflows/registry-readiness.yml/badge.svg)](https://github.com/nazimaraz/TeXRender/actions/workflows/registry-readiness.yml)
[![Static analysis](https://github.com/nazimaraz/TeXRender/actions/workflows/static-analysis.yml/badge.svg)](https://github.com/nazimaraz/TeXRender/actions/workflows/static-analysis.yml)
[![CodeQL](https://github.com/nazimaraz/TeXRender/actions/workflows/codeql.yml/badge.svg)](https://github.com/nazimaraz/TeXRender/actions/workflows/codeql.yml)
[![Thread sanitizer](https://github.com/nazimaraz/TeXRender/actions/workflows/thread-sanitizer.yml/badge.svg)](https://github.com/nazimaraz/TeXRender/actions/workflows/thread-sanitizer.yml)
[![Nightly fuzz](https://github.com/nazimaraz/TeXRender/actions/workflows/nightly-fuzz.yml/badge.svg)](https://github.com/nazimaraz/TeXRender/actions/workflows/nightly-fuzz.yml)
[![Coverage](https://github.com/nazimaraz/TeXRender/actions/workflows/coverage.yml/badge.svg)](https://github.com/nazimaraz/TeXRender/actions/workflows/coverage.yml)
[![Workflow lint](https://github.com/nazimaraz/TeXRender/actions/workflows/workflow-lint.yml/badge.svg)](https://github.com/nazimaraz/TeXRender/actions/workflows/workflow-lint.yml)
[![Web demo](https://github.com/nazimaraz/TeXRender/actions/workflows/web.yml/badge.svg)](https://github.com/nazimaraz/TeXRender/actions/workflows/web.yml)
[![Release smoke](https://github.com/nazimaraz/TeXRender/actions/workflows/release-smoke.yml/badge.svg)](https://github.com/nazimaraz/TeXRender/actions/workflows/release-smoke.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-brightgreen.svg)](docs/api-compatibility.md)
[![CMake package](https://img.shields.io/badge/CMake-package-064f8c.svg)](docs/integration-examples.md#installed-find_package)
[![API docs](https://img.shields.io/badge/API-docs-2b6cb0.svg)](https://nazimaraz.github.io/TeXRender/api/)

Render LaTeX math from C++ into raylib/SDL2 windows, offscreen RGBA buffers, or SVG files.

**[▶ Try the live editor in your browser](https://nazimaraz.github.io/TeXRender/)** —
the same C++ engine compiled to WebAssembly.

[MicroTeX](https://github.com/NanoMichael/MicroTeX) does the hard part — parsing
LaTeX and laying out the boxes — but ships only Cairo / GDI+ / Qt / Skia
backends. Tools like [latexpp](https://github.com/goldsborough/latexpp) only
render to image files. **TeXRender** drives MicroTeX through a small,
pluggable rendering layer so a formula can be drawn live into a window or
rendered offscreen without a GUI dependency.

The default backend is dependency-light **headless** rendering. Opt into the
[raylib](https://www.raylib.com/) or SDL2 backends when you want to draw formulas
inside a caller-owned windowing stack. Dependency-free **headless** and **SVG**
surfaces ship in every build for
offscreen rendering, server output, and tests. The `IDrawSurface` boundary is
proven by raylib, SDL2, software RGBA, and SVG implementations; adding another GUI or
engine backend means implementing one small interface — nothing in the core
changes.

![realtime signal demo](docs/realtime-signal.gif)

*Every frame re-parses, re-lays out, and redraws all five formulas; the overlay
is the measured render rate (rendered offscreen, ~600 FPS here). The GIF itself
is slowed so the math stays readable.*

## Why use TeXRender?

* **No temporary images or subprocesses.** Parse a formula once, keep a
  `RenderHandle`, and draw it inside your normal frame loop.
* **Windowed, raster, and SVG output.** The same API supports raylib overlays,
  SDL2 renderers, server-style headless rendering, self-contained SVG export,
  tests, and WebAssembly.
* **Dependency-shaped CMake.** Consumers can use `FetchContent`,
  `add_subdirectory`, installed `find_package`, vcpkg manifests, or Conan.
* **Small backend contract.** Custom engines only implement `IDrawSurface`; the
  parser, transforms, FreeType glyph rasterization, and caches stay shared.

## Five-minute check

Build the default headless backend and render a PNG without opening a window:

```sh
cmake -S . -B build
cmake --build build -j
./build/texrender_offscreen 'e^{i\pi}+1=0' formula.png
```

Then try the dependency-shaped consumer examples:

```sh
cmake -S examples/consumer-fetchcontent -B build-fetch-example \
  -DTEXRENDER_FETCHCONTENT_SOURCE_DIR="$PWD" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-fetch-example -j
./build-fetch-example/texrender_fetchcontent_example
```

If TeXRender is installed to a prefix:

```sh
cmake -S examples/consumer-find-package -B build-find-example \
  -DCMAKE_PREFIX_PATH=/path/to/texrender-prefix \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-find-example -j
./build-find-example/texrender_find_package_example
```

See the [gallery](docs/gallery.md), [examples index](docs/examples-index.md),
[integration examples](docs/integration-examples.md),
[copyable recipes](docs/recipes.md),
hosted [API reference](https://nazimaraz.github.io/TeXRender/api/),
[why-not-just-TeX FAQ](docs/why-not-just-tex.md),
[backend authoring guide](docs/backend-authoring.md), and
[case study](docs/case-study.md).
The [support matrix](docs/support-matrix.md), [API compatibility notes](docs/api-compatibility.md),
and [migration guide](docs/migration.md) describe the public contract.
The [security threat model](docs/security-threat-model.md),
[dependency/license audit](docs/dependency-license-audit.md), and
[MicroTeX upstream patch note](docs/upstream-microtex-patch.md) cover risk,
redistribution notes, and the one local third-party safety patch. The
[GitHub launch checklist](docs/github-launch.md),
[first public push dry run](docs/first-public-push-dry-run.md),
[local release rehearsal](docs/local-release-rehearsal.md),
[release asset guide](docs/release-assets.md), [registry submission guide](docs/registry-submissions.md),
and [roadmap](docs/roadmap.md) cover the public release path. Support and
community expectations live in [SUPPORT.md](SUPPORT.md) and
[CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md). Resume-focused bullets live in
[`docs/resume.md`](docs/resume.md).

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
                 │  IDrawSurface  ◀── Raylib/SDL2/headless/SVG │ paint primitives
                 └──────────────────────────────────────────┘
        you own the window + render loop (raylib, SDL2, your engine, ...)
```

You drive the window and event loop yourself. For raylib that means `InitWindow`,
`BeginDrawing`/`EndDrawing`, `WindowShouldClose`, `CloseWindow`; for SDL2 that
means your `SDL_Window`/`SDL_Renderer` and event loop. The library never wraps
the application loop; it just paints a parsed formula into the current frame
through the selected `IDrawSurface`.

For a deeper design walk-through, see [`docs/architecture.md`](docs/architecture.md).
For public API and lifetime details, see [`docs/api.md`](docs/api.md).

* **`tex::Graphics2D` is character-based** (`drawChar(wchar_t, x, y)`), drawing by
  Unicode codepoint through each TeX font's cmap. So the integration rasterises
  glyphs from the bundled TeX TrueType fonts with **FreeType** (in `GlyphCache`),
  once, behind a backend-neutral seam.
* **`IDrawSurface`** is the only interface a GUI backend implements — a handful of
  paint primitives in screen pixels. The affine transform, color and glyph
  caching all live in the shared `Graphics2DAdapter`. `RaylibSurface` and
  `Sdl2Surface` are GUI implementations.

## Build

Requirements: a C++17 toolchain and CMake ≥ 3.21. TeXRender itself and its public
headers compile as C++17, and `texrender::texrender` propagates only `cxx_std_17` to
consumers. tinyxml2 and FreeType are resolved with `find_package`, falling back
to `FetchContent`; raylib and SDL2 are resolved only when their backend is
selected. MicroTeX's core is always fetched and compiled from source. On macOS
`brew install freetype tinyxml2` makes the default build instant; add `raylib`
or `sdl2` when building those GUI backends.

```sh
cmake -S . -B build
cmake --build build -j
./build/texrender_offscreen    # render to formula.png — no window needed
./build/texrender_svg_export   # render to formula.svg
ctest --test-dir build      # run the unit tests
```

Windowed raylib demos are opt-in:

```sh
cmake -S . -B build-raylib -DTEXRENDER_BACKEND=raylib
cmake --build build-raylib -j
./build-raylib/texrender_minimal
./build-raylib/texrender_demo
./build-raylib/texrender_realtime_values
```

The smallest possible program is [`examples/minimal.cpp`](examples/minimal.cpp):
parse one formula, draw it in a raylib loop.

More copyable consumer examples live in
[`docs/examples-index.md`](docs/examples-index.md) and
[`docs/integration-examples.md`](docs/integration-examples.md).
For a dependency-shaped wrapper around a parsed formula, see
[`examples/formula_label_widget.cpp`](examples/formula_label_widget.cpp).
For the stable backend-neutral API surface, include `texrender/texrender.hpp`.
Backend-specific headers are opt-in and installed only with packages that export
their matching GUI dependency. Use `texrender/config.hpp` or CMake package
variables such as `texrender_BACKEND` and `texrender_HAS_SDL2` when portable consumer
code needs to inspect package features.

Common contributor builds are also available as CMake presets:

```sh
cmake --preset headless-debug
cmake --build --preset headless-debug
ctest --preset headless-debug
```

See [`docs/build-options.md`](docs/build-options.md) for backend presets,
shared-library builds, SDL2 notes, and the `TEXRENDER_WARNINGS_AS_ERRORS` policy.

## Using TeXRender in your project

### FetchContent / add_subdirectory

TeXRender can build its dependencies (MicroTeX, FreeType, tinyxml2, raylib, SDL2) via
CMake `FetchContent`, so the simplest way to consume it is to fetch the project
the same way — no system packages required:

```cmake
include(FetchContent)

# Pick the default surface before fetching. `headless` is the smallest
# dependency path; use `raylib`, `sdl2`, or `svg` when that is what your app
# wants LatexDocument to create by default.
set(TEXRENDER_BACKEND headless CACHE STRING "" FORCE)
set(TEXRENDER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TEXRENDER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TEXRENDER_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    texrender
    GIT_REPOSITORY https://github.com/nazimaraz/TeXRender.git
    GIT_TAG        v0.1.0        # pin a tag or commit for reproducible builds
)
FetchContent_MakeAvailable(texrender)

target_link_libraries(my_app PRIVATE texrender::texrender)
texrender_copy_resources(my_app)
```

Or, if you vendor the source, `add_subdirectory(path/to/TeXRender)` and link
`texrender::texrender` the same way. Either path skips TeXRender's own examples and tests
automatically when it is built as a subproject. You still drive raylib yourself
or provide your own SDL/custom surface — see [Usage](#usage).

The repository name is `TeXRender`; package-manager IDs, headers, CMake package,
exported target, and pkg-config file use the lowercase `texrender` name
(`find_package(texrender CONFIG REQUIRED)`, `texrender::texrender`,
`#include <texrender/...>`). The C++ namespace is `TeXRender`.

### find_package install

You can also install and consume the exported CMake package:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTEXRENDER_BACKEND=headless
cmake --build build -j
cmake --install build --prefix /path/to/prefix
```

```cmake
find_package(texrender CONFIG REQUIRED COMPONENTS headless svg)

target_link_libraries(my_app PRIVATE texrender::texrender)
texrender_copy_resources(my_app)
```

The installed package uses `find_dependency` for FreeType, tinyxml2 and, when
installed with `TEXRENDER_BACKEND=raylib` or `TEXRENDER_BACKEND=sdl2`, the selected GUI
dependency. Package export is enabled only when those dependencies were found
with `find_package`; if FreeType, tinyxml2, or a GUI dependency is fetched from
source, prefer `FetchContent` / `add_subdirectory` instead of installing that
build tree.

Starter metadata is included for package-manager workflows: [`vcpkg.json`](vcpkg.json)
and [`conanfile.py`](conanfile.py). They are intended as project-local manifests
until the project is submitted to public registries. See
[`docs/package-publishing.md`](docs/package-publishing.md) for the Conan/vcpkg
validation and submission checklist.

Shared-library packages are supported with `BUILD_SHARED_LIBS=ON`; run
[`scripts/verify-shared-library.sh`](scripts/verify-shared-library.sh) to build,
install, and consume a dynamic package from a temporary prefix.

Installed builds also generate `texrender.pc` for pkg-config consumers. For static
linking, use `pkg-config --cflags --libs --static texrender`; the generated
`resdir` variable points at the installed MicroTeX resources for simple
non-CMake tools. A complete Makefile consumer lives in
[`examples/consumer-pkg-config`](examples/consumer-pkg-config).

The public API is pre-1.0 and source-compatibility focused; see
[`docs/api-compatibility.md`](docs/api-compatibility.md) for the current
compatibility promise.

### Shipping the resource directory

MicroTeX needs its `res/` directory (fonts + metrics) **at runtime**; you pass its
path to `LatexDocument`. The CMake helper `texrender_copy_resources(my_app)` copies
that directory to a target-specific folder next to your executable and defines
`TEXRENDER_RES_DIR` for that target:

```cmake
target_link_libraries(my_app PRIVATE texrender::texrender)
texrender_copy_resources(my_app)
```

```cpp
TeXRender::LatexDocument document(TEXRENDER_RES_DIR);
```

If you need custom packaging, copy `TEXRENDER_RESOURCE_DIR` yourself and define your
own resource path macro or configuration value.

## Usage

Pure raylib — you own the window and the loop:

```cpp
#include <raylib.h>
#include <texrender/latex_document.hpp>

InitWindow(640, 360, "LaTeX");

// Scope the document/handle so they are destroyed (freeing GPU glyph textures)
// while the window is still alive — see the lifetime note below.
{
    TeXRender::LatexDocument document("/path/to/MicroTeX/res");
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

For UI rows, `texrender/formula_layout.hpp` provides small helpers such as
`top_for_baseline()`, `draw_at_baseline()` and `draw_aligned()` so formulas can
line up by baseline, center, top, or bottom without redoing metric math.

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
TeXRender::RenderOptions options;
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
    result->draw(40, 40);
}
else
{
    show_error(result.error().message);
}
```

See [`examples/live_edit.cpp`](examples/live_edit.cpp) for a live editor built on
`try_render`.

`try_render()` guardrails are not a process sandbox. For hostile internet input,
run a disposable worker under OS-level timeout and memory limits; see
[`examples/untrusted_worker.cpp`](examples/untrusted_worker.cpp) and the
[`security threat model`](docs/security-threat-model.md).

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
#include <texrender/headless_surface.hpp>
#include <texrender/latex_document.hpp>

auto owned = std::make_unique<TeXRender::HeadlessSurface>();
auto& canvas = *owned;                          // keep a handle; the document takes ownership
TeXRender::LatexDocument document("/path/to/MicroTeX/res", std::move(owned));

auto formula = document.render(R"(\int_0^1 x^2\,dx)", 48.f, 0xff000000);
canvas.resize(formula.width() + 20, formula.height() + 20);
canvas.clear(0xffffffff);                       // opaque white
formula.draw(10, 10);

canvas.write_png("formula.png");                // PNG, PPM, or raw RGBA8 pixels
```

For vector-friendly output, inject `SvgSurface` instead:

```cpp
#include <texrender/svg_surface.hpp>

auto owned = std::make_unique<TeXRender::SvgSurface>();
auto& svg = *owned;
TeXRender::LatexDocument document(TEXRENDER_RES_DIR, std::move(owned));

auto formula = document.render(R"(\sqrt{x+1})", 48.f, 0xff000000);
svg.resize(formula.width() + 20, formula.height() + 20);
svg.clear(0xffffffff);
formula.draw(10, 10);
svg.write_svg("formula.svg");
```

`LatexDocument::surface()` returns the surface a document draws into (the
injected one, or the built backend's default). A runnable version of the above
is [`examples/offscreen.cpp`](examples/offscreen.cpp); SVG export lives in
[`examples/svg_export.cpp`](examples/svg_export.cpp). Both build under every
backend, including `TEXRENDER_BACKEND=headless` and `TEXRENDER_BACKEND=svg`.

## Threading and multiple documents

* **Prefer one rendering thread.** TeXRender serialises shared MicroTeX/glyph-cache
  render paths, and threaded construction/render smoke tests run in CI. GPU
  backends still inherit their toolkit's thread rules, so raylib/SDL2 drawing
  should happen on the render thread.
* **One resource root per process.** MicroTeX initialises from a single global
  resource directory the first time a `LatexDocument` is created, and stays
  initialised for the lifetime of the process (it cannot be safely re-initialised);
  a later document built from a *different* `res_root` reuses the first. Use the
  same `res_root` everywhere. Creating and destroying documents repeatedly is safe.

Set `TEXRENDER_DEBUG=1` to print diagnostics — failed font loads and resource-root
conflicts — to stderr; output is silent otherwise.

Known constraints and tradeoffs are collected in
[`docs/known-limitations.md`](docs/known-limitations.md).

## Adding a backend

Implement `IDrawSurface` from
[`include/texrender/draw_surface.hpp`](include/texrender/draw_surface.hpp) — a handful
of paint primitives plus `draw_glyph` — and inject it into the document. **No
fork required:**

```cpp
TeXRender::LatexDocument document(res_root, std::make_unique<MySurface>());
```

The compact [`HeadlessSurface`](source/backend/headless/headless_surface.cpp) is a
complete, dependency-free raster reference implementation; [`SvgSurface`](source/backend/svg/svg_surface.cpp)
shows the same contract targeting serialized vector output. You keep using your
toolkit's own windowing; only the paint primitives are abstracted. The core,
`Graphics2DAdapter`, and the FreeType glyph cache are reused unchanged.

To ship a backend in-tree instead (selected by `TEXRENDER_BACKEND` as the default
surface), add its sources plus a small `make_surface` factory file under
`source/backend/<name>/` and a branch in `CMakeLists.txt`, the way the raylib
and SDL2 backends do.

## WebAssembly

The raylib backend compiles to WebAssembly, so the live editor runs in the
browser: **<https://nazimaraz.github.io/TeXRender/>**. With the
[Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
activated:

```sh
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release -DTEXRENDER_BACKEND=raylib
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
cmake -S . -B build -DTEXRENDER_BACKEND=headless   # no GPU/display needed
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The suite — transform math, configurable FreeType glyph cache limits,
input-limit/error-diagnostic checks, baseline layout helpers, end-to-end formula
rendering into injected headless and SVG surfaces, PNG/SVG writing, threaded
handle lifetime stress, C++17 public header compatibility, multi-formula visual
regression, and a separate CMake consumer smoke test — needs no window or GPU
and runs under the non-GUI backends; `TEXRENDER_BACKEND=headless` additionally drops
GUI dependencies entirely. CI exercises this on Linux, macOS and Windows, the
raylib backend + examples on Linux and macOS, the SDL2 backend on Linux, runs the
suite under ASan + UBSan, runs PR/scheduled TSan and fuzzing smoke, checks
vcpkg/Conan packaging, verifies release archives and external consumers, verifies
workflow linting and shared-library builds, uploads visual-regression artifacts
on failure, gates formatting with clang-format, runs clang-tidy, and publishes
coverage artifacts.

Additional developer tools:

```sh
cmake --build build --target texrender_render_benchmark -j
./build/texrender_render_benchmark
```

Fuzzing instructions live in [`docs/fuzzing.md`](docs/fuzzing.md); benchmark
notes live in [`docs/benchmarks.md`](docs/benchmarks.md), with reference numbers
in [`docs/performance-baseline.md`](docs/performance-baseline.md). Static
analysis and coverage workflows are documented in
[`docs/static-analysis.md`](docs/static-analysis.md) and
[`docs/coverage.md`](docs/coverage.md). Workflow linting is documented in
[`docs/workflow-linting.md`](docs/workflow-linting.md).
Benchmark tracking is documented in [`docs/benchmarks.md`](docs/benchmarks.md)
and uses [`docs/performance-baseline.json`](docs/performance-baseline.json) for
a loose local regression comparison.
The current platform/backend promise is summarized in
[`docs/support-matrix.md`](docs/support-matrix.md). Release download choices are
explained in [`docs/release-assets.md`](docs/release-assets.md).

## License

TeXRender is released under the [MIT License](LICENSE). It builds on
MicroTeX, raylib, SDL2, FreeType, tinyxml2 and the bundled TeX fonts, each under
its own permissive license or permissive license option. See
[THIRD_PARTY.md](THIRD_PARTY.md) and
[`docs/dependency-license-audit.md`](docs/dependency-license-audit.md) before
redistributing a binary.
