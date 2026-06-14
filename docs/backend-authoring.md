# Backend Authoring Guide

Custom renderers implement `TeXRender::IDrawSurface`. TeXRender handles LaTeX parsing,
box layout, transforms, color state, and FreeType glyph rasterization before
calling your surface.

For the backend-neutral API, consumers can include:

```cpp
#include <texrender/texrender.hpp>
```

Backend headers are opt-in. Installed packages expose `raylib_surface.hpp` only
for raylib builds and `sdl2_surface.hpp` only for SDL2 builds, so package users
do not need GUI headers for headless/SVG installs.

## Draw Contract

- Coordinates are target pixels after texrender has applied MicroTeX transforms.
- Colors arrive through `set_color()` as `0xAARRGGBB`.
- Shape calls are synchronous and should draw immediately unless your graphics
  API requires command recording.
- Rectangle dimensions may be fractional. Backends can snap to pixels or pass
  floats through to their renderer.
- Rounded rectangle radii are advisory. A backend may approximate them when the
  underlying API has limited primitives.
- `draw_glyph()` receives a borrowed grayscale bitmap. The buffer is valid only
  for that draw call.
- `GlyphBitmap::left` and `GlyphBitmap::top` are FreeType bitmap bearings
  relative to the glyph pen.
- `GlyphBitmap::key` is stable across the shared CPU glyph cache and is intended
  for backend-side GPU texture caches.
- `angle` is in radians. If a backend cannot rotate glyphs, document that limit
  the same way `HeadlessSurface` does.

## Lifetime And Threading

`RenderHandle::draw()` calls the surface on the caller's thread. GUI backends
should follow their toolkit's render-thread rules. If the backend owns GPU
resources, release them before the application destroys the graphics context.

`LatexDocument` owns the injected surface through `std::unique_ptr<IDrawSurface>`.
Handles keep document runtime state alive, so graphics-context lifetime is the
application's main cleanup constraint.

## Minimal Shape

```cpp
class MySurface final : public TeXRender::IDrawSurface
{
public:
    void set_color(uint32_t argb) override;
    void draw_line(float x1, float y1, float x2, float y2, float width) override;
    void draw_rect(float x, float y, float w, float h, float width) override;
    void fill_rect(float x, float y, float w, float h) override;
    void draw_round_rect(float x, float y, float w, float h, float rx, float ry, float width) override;
    void fill_round_rect(float x, float y, float w, float h, float rx, float ry) override;
    void draw_glyph(const TeXRender::GlyphBitmap& glyph, float pen_x, float pen_y, float angle) override;
};
```

Inject it when constructing a document:

```cpp
auto surface = std::make_unique<MySurface>();
TeXRender::LatexDocument document(res_root, std::move(surface));
```

## In-Tree Backend Checklist

For a backend maintained in this repository:

- add the public header under `include/texrender/`
- add implementation files under `source/backend/<name>/`
- add a factory file selected by `TEXRENDER_BACKEND`
- update the install header list so the backend header is packaged only when its
  dependencies are exported
- add a public-header C++17 smoke test
- add an example or compile-only package consumer
- document lifetime/toolkit requirements in `docs/known-limitations.md` and
  `docs/support-matrix.md`

Out-of-tree backends usually need only the first code block and a dependency on
`texrender::texrender`.
