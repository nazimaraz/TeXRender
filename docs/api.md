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

Documents are non-copyable and non-movable. Construct and destroy them on the
rendering thread.

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

## `TexGui::RenderHandle`

`RenderHandle` is move-only and owns one parsed/layouted formula. It exposes:

- `width()`, `height()`, `depth()`, `baseline()`
- `set_foreground(argb)`
- `draw(x, y)`

The handle keeps the document's internal state alive, so it can outlive the
`LatexDocument` object that produced it. With GPU-backed surfaces, destroy all
handles before closing the graphics context so cached textures can be released.

## `TexGui::IDrawSurface`

Backends implement a small immediate-mode drawing interface:

- color state: `set_color(argb)`
- rules/shapes: lines, rectangles, rounded rectangles
- glyphs: `draw_glyph(const GlyphBitmap&, pen_x, pen_y, angle)`

Coordinates are screen pixels. `Graphics2DAdapter` handles MicroTeX transforms,
font selection and FreeType rasterization before calls reach the surface.

## Resources

MicroTeX needs the `res/` tree at runtime. In CMake consumers, use:

```cmake
target_link_libraries(my_app PRIVATE texgui::texgui)
texgui_copy_resources(my_app)
```

The helper copies MicroTeX resources to a target-specific directory beside the
executable and defines `TEXGUI_RES_DIR` to that exact path. Then construct
documents with `TEXGUI_RES_DIR`.
