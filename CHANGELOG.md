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
- Two `IDrawSurface` backends: raylib (windowed, GPU glyph textures) and a
  dependency-free headless RGBA canvas; any custom surface can be injected per
  document — `LatexDocument(res_root, std::move(surface))`.
- FreeType glyph rasterisation behind bounded LRU caches (CPU bitmaps and GPU
  textures, keyed identically).
- Examples: minimal window, sample browser, live editor, offscreen render to
  PPM.
- WebAssembly build of the live editor (Emscripten, raylib Web/WebGL, native
  wasm exceptions) with GitHub Pages deployment.
- CI: Linux/macOS/Windows builds and tests, clang-format gate, ASan+UBSan
  pass, wasm build.

[0.1.0]: https://github.com/nazimaraz/latex-renderer/releases/tag/v0.1.0
