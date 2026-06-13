# Resume Notes

Use these as raw material. Pick two or three bullets that match the role instead
of pasting all of them.

## Short Project Summary

Built `latex-renderer`, a C++ LaTeX math rendering library that embeds MicroTeX
inside caller-owned render loops and exposes raylib, SDL2, headless RGBA, and SVG
backends through a small `IDrawSurface` interface.

## Resume Bullets

- Built a C++17 LaTeX math renderer with C++17 public headers, pluggable
  raylib/SDL2/headless/SVG backends, and a backend-neutral FreeType glyph cache.
- Designed a small ownership-oriented API around `LatexDocument`,
  move-only `RenderHandle`, injectable `IDrawSurface`, non-throwing
  `try_render()`, and explicit runtime resource handling.
- Shipped production-style CMake packaging with install exports,
  `find_package(texgui CONFIG REQUIRED)`, `FetchContent`, shared-library support,
  and tested downstream package consumers.
- Added package-manager starter metadata for Conan and vcpkg, including overlay
  smoke tests and release archive validation for dependency-style consumption.
- Built a cross-platform quality pipeline covering Linux/macOS/Windows, ASan,
  UBSan, ThreadSanitizer, fuzz smoke, clang-tidy, CodeQL, coverage, workflow
  linting, WebAssembly, and visual regression artifacts.
- Prepared the project for public open-source release with support policy,
  code-of-conduct, citation metadata, release-asset documentation, and registry
  readiness checks.
- Implemented offscreen PNG/PPM output, self-contained SVG export, generated
  gallery assets, and a GitHub Pages WebAssembly live editor.
- Added dependency-shaped examples, API-level lifetime/cache tests, fuzz seeds,
  and benchmark baseline tracking to make the project reviewable as a library.

## Interview Talking Points

- Why both public headers and implementation files stay compatible with C++17.
- How `IDrawSurface` keeps the core independent of raylib, SDL2, or any future
  renderer.
- Why GUI contexts are borrowed and why examples scope documents before window
  shutdown.
- How package consumers exercise real runtime behavior instead of only linking.
- Why `try_render_detailed()` exists for user-provided input and why it is still
  not a sandbox.
- How release archive smoke prevents “works in checkout, broken in tarball”
  failures.
- How public-project hygiene changes the dependency trust story without changing
  the core library API.

## One-Line Pitch

`latex-renderer` is a dependency-shaped C++ graphics library: small public API,
clear ownership, multiple rendering backends, tested packaging, and public CI
that proves downstream consumption.
