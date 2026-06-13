# Roadmap

This roadmap is intentionally practical. texgui should stay a small rendering
library with a clear embedding story, not grow into a full TeX distribution.

## Near Term

- Validate all GitHub Actions on the public repository after the first push.
- Cut `v0.1.0` and verify release artifacts/checksums.
- Prepare public package submissions:
  - vcpkg port
  - Conan Center recipe
- Add a short README GIF of the live editor.
- Expand Doxygen comments as public API questions come in.
- Track coverage and static-analysis findings from the first real CI runs.
- Validate shared-library builds on public Windows/macOS/Linux CI after push.

## Rendering And API

- Add one more GUI backend as a proof point, likely SDL2 or SFML.
- Add optional SVG export helpers built on the existing `HeadlessSurface` PNG
  output path.
- Add a tiny layout helper for aligning formulas by baseline inside UI rows.
- Improve headless rendering fidelity where it matters for regression testing.
- Consider configurable cache sizes for long-running applications.

## Packaging

- Keep `FetchContent`, `add_subdirectory`, installed CMake, vcpkg, and Conan
  examples working in CI.
- Add release archive smoke tests for all published artifacts.
- Document registry submission steps once vcpkg/Conan Center packages exist.

## Testing And Quality

- Add longer fuzzing campaigns outside normal PR CI.
- Keep sanitizer, clang-tidy, coverage, and visual-regression artifacts green.
- Add small regression cases for any malformed input or rendering issue reported
  by users.

## Non-Goals

- Full TeX document processing.
- Package-level TeX compatibility, page layout, or macro package management.
- Owning the application window, event loop, or GUI framework lifecycle.
- Sandboxing hostile input inside the same process. `try_render()` applies
  guardrails, but high-risk workloads should use process isolation.
- Pixel-perfect equivalence between every backend. Backends should be visually
  consistent, but the headless surface is primarily for tests and offscreen
  output.
