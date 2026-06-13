# Changelog

All notable changes to this project are documented in this file. The format
follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/); versions
follow [Semantic Versioning](https://semver.org/).

## [0.1.0] — 2026-06-13

### Added

- Initial public release: render LaTeX math into a GUI window from C++, driving
  [MicroTeX](https://github.com/NanoMichael/MicroTeX) through a pluggable
  rendering layer.
- `LatexDocument` / `RenderHandle` public API: throwing `render()` and
  `noexcept try_render()` for untrusted input, UTF-8 LaTeX strings, metrics
  (`width`/`height`/`depth`/`baseline`), recolouring, optional wrap width.
- Baseline and anchor layout helpers in `texgui/formula_layout.hpp`.
- `try_render_detailed()`, `RenderResult` and `RenderErrorCode` for callers that
  need non-throwing diagnostics instead of only `std::optional`.
- `RenderOptions` for bounded untrusted-input rendering; the simple
  `try_render()` overload applies default input, text-size and wrap-width limits.
- Three `IDrawSurface` backends: raylib (windowed, GPU glyph textures), SDL2
  (`SDL_Renderer` integration), and a dependency-free headless RGBA canvas; any
  custom surface can be injected per document —
  `LatexDocument(res_root, std::move(surface))`.
- `SvgSurface` and `TEXGUI_BACKEND=svg` for self-contained SVG output with
  vector shape primitives and embedded glyph PNGs.
- `HeadlessSurface::write_png()` for dependency-free PNG output alongside PPM
  and raw RGBA pixels.
- `CacheLimits`, `cache_limits()` and `set_cache_limits()` for CPU glyph and
  backend texture cache sizing.
- Shared-library symbol visibility via `TEXGUI_EXPORT`, including Windows DLL
  import/export definitions.
- `CMakePresets.json` for headless, raylib, SDL2, shared, sanitizer, coverage
  and fuzzing builds.
- `TEXGUI_WARNINGS_AS_ERRORS` for first-party warning gates without applying
  warning policy to bundled third-party sources.
- CMake package support: `install()`, exported `texgui::texgui`, generated
  `texguiConfig.cmake`, and `texgui_copy_resources(target)`.
- Package-manager starter metadata for vcpkg and Conan consumers.
- Dependabot configuration for GitHub Actions dependency updates.
- Conan `shared`/`fPIC` package options and generated CMake build-module
  metadata for resource-copy helpers.
- Local vcpkg overlay port smoke test for portfile-style package validation,
  with pinned MicroTeX source handoff for disconnected configure.
- CPack source and binary archive configuration with local extraction/consumer
  smoke validation.
- FreeType glyph rasterisation behind bounded LRU caches (CPU bitmaps and GPU
  textures, keyed identically).
- Examples: minimal window, sample browser, live editor, offscreen render to
  PNG/PPM, SVG export, headless batch output, raylib overlay, SDL2 minimal,
  custom surface, and copyable FetchContent / installed-package consumer
  projects.
- Benchmark executable for cold render, warm render and headless draw timing.
- libFuzzer harness for `try_render()` with sanitizer-friendly input limits.
- Seeded fuzz corpus and scheduled nightly fuzz workflow for longer malformed
  input coverage outside normal PR CI.
- WebAssembly build of the live editor (Emscripten, raylib Web/WebGL, native
  wasm exceptions) with GitHub Pages deployment.
- Live-editor screenshot capture controls and a reproducible README GIF
  recording script.
- Generated Doxygen API reference published with the web demo.
- Release automation for tagged GitHub releases with install archives and
  SHA256 checksums, CPack archives, plus grouped generated release notes.
- Public launch helper script that checks clean-tree/remote state, runs archive
  smoke, and optionally creates the local release tag.
- CI/test coverage: Linux/macOS/Windows builds, C++17 consumer smoke test,
  installed package consumer, C++17 public-header checks, strict warning build,
  SVG and SDL2 backend smoke, Windows shared-library install consumer, shared-library
  smoke, threaded lifetime stress test, multi-formula structural visual
  regression, optional exact PNG visual snapshots, visual failure artifacts,
  clang-format gate, ASan+UBSan pass, fuzz smoke, vcpkg/Conan validation,
  clang-tidy and CodeQL static analysis, coverage reporting, actionlint workflow
  linting, non-blocking benchmark output, wasm build.
- Generated gallery manifest/script that renders PNG and SVG sample artifacts in
  CI and release verification.
- Local release verifier, CPack smoke, source archive smoke, external consumer
  smoke, path-with-spaces smoke, Markdown link checker, and registry-readiness
  checker for format/build/test/docs/package-consumer/package-metadata checks
  before pushing.
- Documentation: architecture, API notes, known limitations, integration
  examples, gallery, build options, fuzzing, benchmarks, static analysis,
  coverage, workflow linting, API compatibility, migration guide, GitHub launch, package
  publishing, registry submissions, support matrix, v0.1.0 release notes,
  roadmap, performance baseline, resume notes, resume-oriented case study and
  release checklist.

[0.1.0]: https://github.com/nazimaraz/latex-renderer/releases/tag/v0.1.0
