# TeXRender API Reference

TeXRender renders LaTeX math formulas into a caller-owned rendering loop. The
primary public types are small:

- `TeXRender::LatexDocument` owns renderer state and one `IDrawSurface`.
- `TeXRender::RenderHandle` owns one parsed and laid-out formula.
- `TeXRender::RenderOptions` bounds untrusted input before it reaches MicroTeX.
- `TeXRender::RenderResult` and `TeXRender::RenderError` preserve diagnostics for
  non-throwing caller-input rendering.
- `TeXRender::IDrawSurface` is the backend interface for custom render targets.
- `TeXRender::HeadlessSurface` is the software RGBA backend for tests, servers, and
  PNG/PPM offscreen output.
- `TeXRender::RaylibSurface` is the raylib-backed GPU surface.
- `TeXRender::Sdl2Surface` is the SDL2-backed GPU surface.
- `TeXRender::SvgSurface` is the self-contained SVG output surface.
- `TeXRender::CacheLimits` controls CPU glyph and backend texture cache sizes.
- `texrender/formula_layout.hpp` provides baseline and anchor helpers.

Start with `TeXRender::LatexDocument`, then follow the surface type that matches
your embedding mode.

See also:

- [Embedding notes](api.md)
- [API compatibility](api-compatibility.md)
- [Security threat model](security-threat-model.md)
- [Dependency and license audit](dependency-license-audit.md)
- [Upstream MicroTeX patch note](upstream-microtex-patch.md)
- [Migration guide](migration.md)
- [Architecture](architecture.md)
- [Backend authoring guide](backend-authoring.md)
- [Why not just TeX?](why-not-just-tex.md)
- [Known limitations](known-limitations.md)
- [Examples index](examples-index.md)
- [Integration recipes](recipes.md)
- [Build options](build-options.md)
- [GitHub launch checklist](github-launch.md)
- [First public push dry run](first-public-push-dry-run.md)
- [Local release rehearsal](local-release-rehearsal.md)
- [v0.1.0 release body](release-body-v0.1.0.md)
- [Release assets](release-assets.md)
- [Release checklist](release-checklist.md)
- [Package publishing](package-publishing.md)
- [Registry submissions](registry-submissions.md)
- [Support matrix](support-matrix.md)
- [v0.1.0 release notes](release-notes-v0.1.0.md)
- [Resume notes](resume.md)
- [Static analysis](static-analysis.md)
- [Coverage](coverage.md)
- [Workflow linting](workflow-linting.md)
- [Roadmap](roadmap.md)
- [Case study](case-study.md)
