# texgui API Reference

texgui renders LaTeX math formulas into a caller-owned rendering loop. The
primary public types are small:

- `TexGui::LatexDocument` owns renderer state and one `IDrawSurface`.
- `TexGui::RenderHandle` owns one parsed and laid-out formula.
- `TexGui::RenderOptions` bounds untrusted input before it reaches MicroTeX.
- `TexGui::RenderResult` and `TexGui::RenderError` preserve diagnostics for
  non-throwing caller-input rendering.
- `TexGui::IDrawSurface` is the backend interface for custom render targets.
- `TexGui::HeadlessSurface` is the software RGBA backend for tests, servers, and
  PNG/PPM offscreen output.
- `TexGui::RaylibSurface` is the raylib-backed GPU surface.
- `TexGui::Sdl2Surface` is the SDL2-backed GPU surface.
- `TexGui::SvgSurface` is the self-contained SVG output surface.
- `TexGui::CacheLimits` controls CPU glyph and backend texture cache sizes.
- `texgui/formula_layout.hpp` provides baseline and anchor helpers.

Start with `TexGui::LatexDocument`, then follow the surface type that matches
your embedding mode.

See also:

- [Embedding notes](api.md)
- [API compatibility](api-compatibility.md)
- [Migration guide](migration.md)
- [Architecture](architecture.md)
- [Why not just TeX?](why-not-just-tex.md)
- [Known limitations](known-limitations.md)
- [Examples index](examples-index.md)
- [Build options](build-options.md)
- [GitHub launch checklist](github-launch.md)
- [Local release rehearsal](local-release-rehearsal.md)
- [v0.1.0 release body](release-body-v0.1.0.md)
- [Release assets](release-assets.md)
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
