# texgui API Reference

texgui renders LaTeX math formulas into a caller-owned rendering loop. The
primary public types are small:

- `TexGui::LatexDocument` owns renderer state and one `IDrawSurface`.
- `TexGui::RenderHandle` owns one parsed and laid-out formula.
- `TexGui::RenderOptions` bounds untrusted input before it reaches MicroTeX.
- `TexGui::IDrawSurface` is the backend interface for custom render targets.
- `TexGui::HeadlessSurface` is the software RGBA backend for tests, servers, and
  offscreen output.
- `TexGui::RaylibSurface` is the raylib-backed GPU surface.

Start with `TexGui::LatexDocument`, then follow the surface type that matches
your embedding mode.

See also:

- [Embedding notes](api.md)
- [Architecture](architecture.md)
- [Known limitations](known-limitations.md)
- [Static analysis](static-analysis.md)
- [Coverage](coverage.md)
- [Workflow linting](workflow-linting.md)
- [Roadmap](roadmap.md)
- [Case study](case-study.md)
