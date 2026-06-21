# Release Notes: v0.1.0

`v0.1.0` is the first public release candidate for texrender, a small C++ library
for embedding LaTeX math rendering into applications.

## Highlights

- LaTeX math rendering through MicroTeX with a small `IDrawSurface` backend
  interface.
- GUI backends for raylib and SDL2.
- Dependency-free headless RGBA surface with PNG/PPM output.
- Self-contained SVG export surface.
- CMake consumption through `FetchContent`, `add_subdirectory`, installed
  `find_package(texrender CONFIG REQUIRED)`, installed pkg-config metadata, and
  package-manager starter metadata.
- Implementation and public headers validated as C++17.
- Backend-neutral umbrella header `texrender/texrender.hpp`, with installed packages
  exposing only the backend headers their dependencies support.
- Generated `texrender/config.hpp`, CMake package feature metadata, and CMake
  package components for backend-aware consumers.
- Non-throwing `try_render()` and `try_render_detailed()` paths for
  caller-supplied formulas.
- Explicit public ownership contracts: non-copyable documents/surfaces,
  move-only handles, `noexcept` cleanup, metrics, and surface inspection helpers.
- CI coverage for Linux, macOS, Windows, sanitizers, ThreadSanitizer, fuzz
  smoke, nightly seeded fuzzing, static analysis, package consumers, registry
  metadata, release archive smoke, CPack archive smoke, external consumer smoke,
  path-with-spaces smoke, and the WebAssembly live editor.
- CI docs gates for Doxygen API generation and third-party notice drift.
- Public project hygiene: support policy, code of conduct, citation metadata,
  CODEOWNERS, release asset guide, and a why-not-just-TeX FAQ.
- Documented local MicroTeX safety patch with an upstream issue/PR draft.
- Fresh-clone release rehearsal and local cleanup helpers for final pre-push
  validation.
- Public API snapshot and pkg-config consumer smoke checks for dependency-facing
  changes.
- Installed-header and optional installed-backend package smoke checks for
  dependency-facing package exports.
- Package component success/failure smoke checks for installed package metadata.
- Installed shared-library package smoke checks for dynamic-linking consumers.
- Copyable integration recipes for HUD rendering, PNG/SVG generation, custom
  surfaces, safe user input, and portable feature checks.
- A dependency-shaped formula-label example that wraps a parsed `RenderHandle`
  inside a reusable application object.
- Expanded structural visual regression formulas for nested fractions, large
  operators, spacing commands, symbols, and colored rules.
- Expanded fuzz corpus seeds for malformed matrices, accents, Unicode, colors,
  spacing commands, large operators, and nested scripts.
- Benchmark baseline comparison in warn-only mode for visible performance drift.
- Security threat model, dependency/license audit, and first public push dry-run
  checklist for public launch review.

## Stability

The public API is pre-1.0 and source-compatibility focused. Patch releases in
the `0.1.x` line should keep documented public headers source-compatible unless
a bug fix requires a clearly documented break. ABI stability is not promised
before `1.0`.

## Known Limits

- TeXRender renders formulas, not full TeX documents.
- MicroTeX resources are required at runtime.
- GUI backends inherit their toolkit lifetime/threading rules.
- `try_render()` guardrails are not a security sandbox.
- SDL2 rounded rectangle calls currently fall back to regular rectangles.

## Suggested Release Assets

- Source archive and SHA256 checksum from `scripts/verify-release-archive.sh`.
- Source/binary CPack archives after `scripts/verify-cpack.sh` is green.
- Release asset guide: [`docs/release-assets.md`](release-assets.md).
- Release body draft: [`docs/release-body-v0.1.0.md`](release-body-v0.1.0.md).
- GitHub Pages live editor URL.
- README live-editor GIF refreshed with `scripts/record-live-editor-demo.sh`.
- Generated API reference.
- README/gallery images generated from the release commit.
