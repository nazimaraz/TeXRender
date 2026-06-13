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
- `RenderOptions` for bounded untrusted-input rendering; the simple
  `try_render()` overload applies default input, text-size and wrap-width limits.
- Two `IDrawSurface` backends: raylib (windowed, GPU glyph textures) and a
  dependency-free headless RGBA canvas; any custom surface can be injected per
  document — `LatexDocument(res_root, std::move(surface))`.
- CMake package support: `install()`, exported `texgui::texgui`, generated
  `texguiConfig.cmake`, and `texgui_copy_resources(target)`.
- Package-manager starter metadata for vcpkg and Conan consumers.
- FreeType glyph rasterisation behind bounded LRU caches (CPU bitmaps and GPU
  textures, keyed identically).
- Examples: minimal window, sample browser, live editor, offscreen render to
  PPM, headless batch output, raylib overlay, and copyable FetchContent /
  installed-package consumer projects.
- Benchmark executable for cold render, warm render and headless draw timing.
- libFuzzer harness for `try_render()` with sanitizer-friendly input limits.
- WebAssembly build of the live editor (Emscripten, raylib Web/WebGL, native
  wasm exceptions) with GitHub Pages deployment.
- Generated Doxygen API reference published with the web demo.
- Release automation for tagged GitHub releases with install archives and
  SHA256 checksums plus grouped generated release notes.
- CI/test coverage: Linux/macOS/Windows builds, C++17 consumer smoke test,
  installed package consumer, strict warning build, structural visual regression,
  visual failure artifacts, clang-format gate, ASan+UBSan pass, fuzz smoke,
  vcpkg/Conan validation, clang-tidy static analysis, coverage reporting,
  actionlint workflow linting, non-blocking benchmark output, wasm build.
- Local release verifier script for format/build/test/docs/package-consumer
  checks before pushing.
- Documentation: architecture, API notes, known limitations, integration
  examples, gallery, fuzzing, benchmarks, static analysis, coverage,
  workflow linting, roadmap, performance baseline, resume-oriented case study
  and release checklist.

[0.1.0]: https://github.com/nazimaraz/latex-renderer/releases/tag/v0.1.0
