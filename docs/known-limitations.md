# Known Limitations

TeXRender is intentionally small. These constraints are part of the current design
and are worth knowing before embedding it.

## Not A Full TeX Engine

The library renders math formulas through MicroTeX. It is not a document
processor and does not aim to implement the full TeX ecosystem, packages or page
layout.

## One Resource Root Per Process

MicroTeX initializes its resource directory once. The first `LatexDocument`
chooses the process-wide resource root; later documents should use the same path.
Different paths are ignored and can be reported with `TEXRENDER_DEBUG=1`.

`TeXRender::shutdown()` is currently a safe, idempotent compatibility hook. It
does not release MicroTeX's process-wide parser/font tables because the pinned
upstream teardown path is not safe to re-initialize afterward. Calling it too
early is a no-op, so surviving handles remain usable.

## Threading Boundary

Shared MicroTeX/glyph-cache render paths are serialized, and repeated threaded
document creation/render smoke tests are covered. GUI backends still inherit
their toolkit rules: raylib drawing should happen on the render thread.

## Raylib Lifetime

With `RaylibSurface`, GPU textures are released when the last
`LatexDocument`/`RenderHandle` using that surface is destroyed. Destroy those
objects before `CloseWindow()`.

## SDL2 Lifetime And Rounded Rectangles

`Sdl2Surface` borrows an `SDL_Renderer`. Destroy TeXRender documents and handles
before destroying the renderer so cached SDL textures can be released safely.
SDL2 does not provide rounded rectangle primitives, so `Sdl2Surface` currently
falls back to regular rectangles for rounded rectangle draw calls.

## Resource Guardrails Are Not A Sandbox

`try_render()` applies input-size and layout guardrails for untrusted text, but it
does not sandbox MicroTeX. Applications that accept internet-scale arbitrary
input should keep their own request limits and consider process isolation for
high-risk workloads.

## Package Export With Fetched Dependencies

Installed CMake package export works when FreeType, tinyxml2, and any selected
GUI backend were found with `find_package()`. If one of those dependencies is
built from a submodule, `TEXRENDER_INSTALL`
defaults off because the installed package would otherwise reference build-tree
targets that consumers cannot recreate reliably.

## Headless Rendering Fidelity

`HeadlessSurface` is designed for CI, offscreen output and regression testing. It
uses software alpha compositing, writes PNG/PPM output, and ignores glyph
rotation, so it is not meant to be a perfect reference for every backend effect.
`SvgSurface` writes SVG primitives for shapes but embeds glyphs as PNG data URIs
because glyphs are already rasterized by the time draw calls reach surfaces.
