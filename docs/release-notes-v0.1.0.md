# Release Notes: v0.1.0

`v0.1.0` is the first public release candidate for texgui, a small C++ library
for embedding LaTeX math rendering into applications.

## Highlights

- LaTeX math rendering through MicroTeX with a small `IDrawSurface` backend
  interface.
- GUI backends for raylib and SDL2.
- Dependency-free headless RGBA surface with PNG/PPM output.
- Self-contained SVG export surface.
- CMake consumption through `FetchContent`, `add_subdirectory`, installed
  `find_package(texgui CONFIG REQUIRED)`, and package-manager starter metadata.
- Public headers validated as C++17 while the implementation can use modern
  C++23.
- Non-throwing `try_render()` and `try_render_detailed()` paths for
  caller-supplied formulas.
- CI coverage for Linux, macOS, Windows, sanitizers, fuzz smoke, static analysis,
  package consumers, release archive smoke, CPack archive smoke, and the
  WebAssembly live editor.

## Stability

The public API is pre-1.0 and source-compatibility focused. Patch releases in
the `0.1.x` line should keep documented public headers source-compatible unless
a bug fix requires a clearly documented break. ABI stability is not promised
before `1.0`.

## Known Limits

- texgui renders formulas, not full TeX documents.
- MicroTeX resources are required at runtime.
- GUI backends inherit their toolkit lifetime/threading rules.
- `try_render()` guardrails are not a security sandbox.
- SDL2 rounded rectangle calls currently fall back to regular rectangles.

## Suggested Release Assets

- Source archive and SHA256 checksum from `scripts/verify-release-archive.sh`.
- Source/binary CPack archives after `scripts/verify-cpack.sh` is green.
- GitHub Pages live editor URL.
- README live-editor GIF refreshed with `scripts/record-live-editor-demo.sh`.
- Generated API reference.
- README/gallery images generated from the release commit.
