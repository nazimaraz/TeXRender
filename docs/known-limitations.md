# Known Limitations

texgui is intentionally small. These constraints are part of the current design
and are worth knowing before embedding it.

## Not A Full TeX Engine

The library renders math formulas through MicroTeX. It is not a document
processor and does not aim to implement the full TeX ecosystem, packages or page
layout.

## One Resource Root Per Process

MicroTeX initializes its resource directory once. The first `LatexDocument`
chooses the process-wide resource root; later documents should use the same path.
Different paths are ignored and can be reported with `TEXGUI_DEBUG=1`.

## Threading Boundary

Shared MicroTeX/glyph-cache render paths are serialized, and repeated threaded
document creation/render smoke tests are covered. GUI backends still inherit
their toolkit rules: raylib drawing should happen on the render thread.

## Raylib Lifetime

With `RaylibSurface`, GPU textures are released when the last
`LatexDocument`/`RenderHandle` using that surface is destroyed. Destroy those
objects before `CloseWindow()`.

## Resource Guardrails Are Not A Sandbox

`try_render()` applies input-size and layout guardrails for untrusted text, but it
does not sandbox MicroTeX. Applications that accept internet-scale arbitrary
input should keep their own request limits and consider process isolation for
high-risk workloads.

## Package Export With Fetched Raylib

Installed CMake package export works for the headless and SVG backends, and for
raylib when raylib was found with `find_package(raylib)`. If raylib is pulled in
through `FetchContent`, `TEXGUI_INSTALL` defaults off because exporting another
project's build-tree target would make a fragile package.

## Headless Rendering Fidelity

`HeadlessSurface` is designed for CI, offscreen output and regression testing. It
uses software alpha compositing, writes PNG/PPM output, and ignores glyph
rotation, so it is not meant to be a perfect reference for every backend effect.
`SvgSurface` writes SVG primitives for shapes but embeds glyphs as PNG data URIs
because glyphs are already rasterized by the time draw calls reach surfaces.
