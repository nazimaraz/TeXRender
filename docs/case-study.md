# Case Study: Building texgui

texgui started with a narrow problem: render LaTeX math directly inside a C++
application frame loop, without spawning TeX, rasterizing temporary files, or
owning the caller's windowing code.

## Problem

MicroTeX can parse and lay out math formulas, but its public rendering backends
target toolkits like Cairo, GDI+, Qt, and Skia. raylib applications need
something different: a formula should behave like any other drawable object in
the frame. The application still owns `InitWindow()`, `BeginDrawing()`,
`EndDrawing()`, and `CloseWindow()`.

The library therefore separates three jobs:

- MicroTeX parses and lays out formulas.
- texgui adapts MicroTeX's `tex::Graphics2D` calls into backend-neutral drawing.
- A small `IDrawSurface` implementation paints into raylib, a software canvas, or
  a future user-provided backend.

## C++ Design

The public API is deliberately small:

- `LatexDocument` owns renderer state and one drawing surface.
- `RenderHandle` is move-only and owns one parsed/layouted formula.
- `RenderOptions` provides guardrails for caller-supplied input.
- `IDrawSurface` is the extension point for custom backends.

Ownership is explicit. `LatexDocument` owns the surface, and `RenderHandle`
shares the internal document state so handles remain valid even if the document
object is released first. GPU-backed surfaces still have a real graphics-context
lifetime, so examples scope handles before `CloseWindow()`.

The core uses modern C++ where it helps:

- RAII for document state, glyph textures, and offscreen buffers
- move-only handles for parsed formulas
- `std::unique_ptr` injection for custom surfaces
- `std::optional` for non-throwing untrusted-input rendering
- `std::shared_ptr` only at the document/handle lifetime boundary
- C++17 public headers, while implementation files can use newer C++ features

## Rendering Pipeline

MicroTeX issues high-level drawing commands through `tex::Graphics2D`.
`Graphics2DAdapter` translates that into:

- transform tracking
- current color state
- FreeType glyph rasterization
- backend-neutral `IDrawSurface` calls

The raylib backend uploads glyph bitmaps to textures and keeps them in an LRU
cache. The headless backend composites straight RGBA pixels in software, which
makes tests, CI, and server-side output possible without a GPU.

## Constraints

The project intentionally documents its constraints instead of hiding them:

- MicroTeX initializes one resource root per process.
- Rendering is single-threaded.
- `try_render()` adds guardrails but is not a sandbox.
- Installed package export is simplest with the headless backend or an installed
  raylib package.
- Headless output is stable enough for tests but not a perfect visual reference
  for every GPU backend effect.

Those tradeoffs are collected in
[`docs/known-limitations.md`](known-limitations.md).

## Testing Strategy

The test suite avoids a window or GPU by injecting `HeadlessSurface`.

Coverage includes:

- transform math
- FreeType glyph cache behavior
- public API render and error paths
- untrusted input guardrails
- software compositing
- structural visual regression
- CMake consumer smoke tests
- installed package consumer tests

CI extends that with ASan/UBSan, fuzz smoke, strict warnings, package-manager
validation, static analysis, coverage artifacts, WebAssembly builds, and release
artifact generation.

## Packaging Story

texgui supports the ways C++ developers commonly consume a library:

- `FetchContent`
- `add_subdirectory`
- installed `find_package(texgui CONFIG REQUIRED)`
- vcpkg manifest mode
- Conan 2 recipe and `test_package`

MicroTeX's `res/` directory is a runtime dependency. The CMake helper
`texgui_copy_resources(target)` copies it to a target-specific output directory
and defines `TEXGUI_RES_DIR` for that target.

## What This Demonstrates

For a resume review, this project demonstrates:

- practical CMake packaging and install exports
- third-party source integration through `FetchContent`
- graphics backend abstraction without owning the application's window loop
- API design with clear ownership and lifetime rules
- testability through dependency injection
- sanitizer, fuzz, coverage, and static-analysis workflows
- WebAssembly deployment of native C++ code
- release automation with checksums and install artifacts
