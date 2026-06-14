# Why Not Just TeX?

TeXRender exists for applications that need LaTeX-style math inside an
existing C++ render loop. It is not trying to replace TeX, KaTeX, MathJax, or a
document renderer.

## Why not shell out to LaTeX?

Calling a TeX distribution in a subprocess is excellent for documents and exact
typesetting, but it is awkward for interactive applications:

- process startup and file I/O make frame-by-frame rendering expensive
- temporary files complicate sandboxing and cleanup
- runtime dependencies are large for games, tools, and embedded demos
- the output usually arrives as an image or PDF rather than draw calls for your
  renderer

TeXRender keeps parsed formulas as `RenderHandle` objects and draws them
into the current frame through `IDrawSurface`.

## Why not KaTeX or MathJax?

KaTeX and MathJax are excellent choices for browser HTML. TeXRender is for
C++ applications that want native rendering backends, CMake packaging, and direct
integration with raylib, SDL2, custom engines, offscreen RGBA output, or SVG
export.

The WebAssembly demo exists to prove the same C++ code can run in a browser, not
because the project wants to compete with browser-native math layout.

## Why MicroTeX?

MicroTeX already handles formula parsing and box layout. This project builds the
C++ integration layer around it:

- public `LatexDocument` / `RenderHandle` API
- runtime resource handling
- FreeType glyph rasterization
- backend-neutral draw surface
- raylib, SDL2, headless, and SVG surfaces
- install/export/package-manager behavior
- tests and release validation

That lets the project focus on embedding and packaging instead of inventing a
new TeX parser.

## What is intentionally out of scope?

- full TeX documents
- page layout
- TeX packages
- exact browser DOM output
- replacing the caller's window or event loop
- sandboxing hostile input in the same process

For the detailed boundary, see [Known limitations](known-limitations.md) and
[API compatibility](api-compatibility.md).
