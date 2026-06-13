# Roadmap

This roadmap is intentionally practical. texgui should stay a small rendering
library with a clear embedding story, not grow into a full TeX distribution.

## Near Term

- Validate all GitHub Actions on the public repository after the first push.
- Cut `v0.1.0` and verify release artifacts/checksums.
- Prepare actual public registry submissions:
  - vcpkg port
  - Conan Center recipe
- Add a short README GIF of the live editor.
- Expand Doxygen comments as public API questions come in.
- Track coverage and static-analysis findings from the first real CI runs.
- Review first Dependabot PRs and tune grouping/labels if needed.

## Rendering And API

- Improve headless rendering fidelity where it matters for regression testing.
- Add one more GUI backend as a proof point, likely SDL2 or SFML.
- Consider compressed PNG output if file size becomes important for SVG glyph
  data URIs.

## Packaging

- Keep `FetchContent`, `add_subdirectory`, installed CMake, vcpkg, and Conan
  examples working in CI.
- Add release archive smoke tests for all published artifacts.
- Keep `docs/package-publishing.md` aligned with registry requirements.

## Testing And Quality

- Add longer fuzzing campaigns outside normal PR CI.
- Keep sanitizer, clang-tidy, coverage, and visual-regression artifacts green.
- Promote optional visual snapshots to committed goldens if a platform-stable
  image baseline becomes useful.
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
