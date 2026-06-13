# API Notes

This is the practical contract for embedding texgui in an application.

## `TexGui::LatexDocument`

`LatexDocument` owns the renderer state and exactly one `IDrawSurface`.

```cpp
TexGui::LatexDocument document(TEXGUI_RES_DIR);
```

By default the document creates the backend selected at build time
(`TEXGUI_BACKEND`). To render somewhere else, inject a surface:

```cpp
auto owned = std::make_unique<TexGui::HeadlessSurface>();
auto& canvas = *owned;
TexGui::LatexDocument document(TEXGUI_RES_DIR, std::move(owned));
```

Documents are non-copyable and non-movable. texgui serializes shared
MicroTeX/glyph-cache render paths, but GPU toolkits still have their own thread
rules; draw raylib-backed documents on the render thread.

## `render()` and `try_render()`

Use `render()` for trusted formulas:

```cpp
auto formula = document.render(R"(\sqrt{x+1})", 48.f, 0xff000000);
```

It throws a `std::exception`-derived error if MicroTeX cannot parse the input or
if supplied `RenderOptions` are invalid.

Use `try_render()` for caller-supplied input:

```cpp
if (auto formula = document.try_render(user_input, 48.f, 0xff000000))
    formula->draw(20, 20);
```

The simple `try_render()` overload applies default guardrails:

- `max_input_bytes`: 64 KiB
- `max_text_size`: 512 px
- `max_wrap_width`: 32768 px

Tune those with `RenderOptions`:

```cpp
TexGui::RenderOptions options;
options.text_size = 42.f;
options.argb = 0xff202020;
options.wrap_width = 480;
options.max_input_bytes = 8192;
options.max_text_size = 96.f;
options.max_wrap_width = 1024;

auto formula = document.try_render(user_input, options);
```

Set a limit field to its `no_*_limit` constant only for trusted input.

Use `try_render_detailed()` when the caller needs a failure reason without
exceptions:

```cpp
auto result = document.try_render_detailed(user_input, options);
if (!result)
{
    switch (result.error().code)
    {
    case TexGui::RenderErrorCode::parse_error:
        show_parse_error(result.error().message);
        break;
    case TexGui::RenderErrorCode::input_too_large:
        show_parse_error("Formula is too large");
        break;
    default:
        show_parse_error(result.error().message);
        break;
    }
}
```

`RenderErrorCode` distinguishes invalid options, input-size limits,
text-size/wrap-width limits, MicroTeX parse errors and unexpected failures.
The original `try_render()` remains the compact `std::optional<RenderHandle>`
wrapper around the same implementation.

## `TexGui::RenderHandle`

`RenderHandle` is move-only and owns one parsed/layouted formula. It exposes:

- `width()`, `height()`, `depth()`, `baseline()`
- `set_foreground(argb)`
- `draw(x, y)`

The handle keeps the document's internal state alive, so it can outlive the
`LatexDocument` object that produced it. With GPU-backed surfaces, destroy all
handles before closing the graphics context so cached textures can be released.

For alignment, include `texgui/formula_layout.hpp`:

```cpp
const auto y = TexGui::top_for_baseline(formula, row_baseline);
formula.draw(x, y);
TexGui::draw_aligned(formula, x, center_y, TexGui::FormulaAnchor::center);
```

## `TexGui::IDrawSurface`

Backends implement a small immediate-mode drawing interface:

- color state: `set_color(argb)`
- rules/shapes: lines, rectangles, rounded rectangles
- glyphs: `draw_glyph(const GlyphBitmap&, pen_x, pen_y, angle)`

Coordinates are screen pixels. `Graphics2DAdapter` handles MicroTeX transforms,
font selection and FreeType rasterization before calls reach the surface.

`HeadlessSurface` stores straight RGBA8 pixels and can write either `PPM` or
`PNG` files:

```cpp
canvas.write_png("formula.png");
canvas.write_ppm("formula.ppm");
```

PNG writing is built in and does not require an image library dependency.

`SvgSurface` records the same draw calls into a self-contained SVG document:

```cpp
auto owned = std::make_unique<TexGui::SvgSurface>();
auto& svg = *owned;
TexGui::LatexDocument document(TEXGUI_RES_DIR, std::move(owned));
auto formula = document.render(R"(\sqrt{x+1})", 48.f, 0xff000000);
svg.resize(formula.width() + 20, formula.height() + 20);
svg.clear(0xffffffff);
formula.draw(10, 10);
svg.write_svg("formula.svg");
```

Shapes are emitted as SVG primitives. Glyphs are FreeType bitmaps embedded as
small PNG data URIs because MicroTeX's drawing API reaches backends after glyph
rasterization.

## Cache Limits

The shared CPU glyph cache and default backend texture cache limits are
configurable:

```cpp
auto limits = TexGui::cache_limits();
limits.max_cpu_glyphs = 2048;
limits.max_gpu_textures = 512;
TexGui::set_cache_limits(limits);
```

`RaylibSurface::set_max_textures()` can override the GPU texture limit for one
surface.

## SDL2 Backend

`Sdl2Surface` renders into a borrowed `SDL_Renderer`. Register the renderer once
for the default backend path:

```cpp
TexGui::set_default_sdl_renderer(renderer);
TexGui::LatexDocument document(TEXGUI_RES_DIR);
```

or inject a surface explicitly:

```cpp
auto owned = std::make_unique<TexGui::Sdl2Surface>(renderer);
TexGui::LatexDocument document(TEXGUI_RES_DIR, std::move(owned));
```

Like the raylib backend, SDL textures are cached per surface. Destroy formulas
and documents before destroying the SDL renderer.

## Resources

MicroTeX needs the `res/` tree at runtime. In CMake consumers, use:

```cmake
target_link_libraries(my_app PRIVATE texgui::texgui)
texgui_copy_resources(my_app)
```

The helper copies MicroTeX resources to a target-specific directory beside the
executable and defines `TEXGUI_RES_DIR` to that exact path. Then construct
documents with `TEXGUI_RES_DIR`.
