# Integration Recipes

These recipes are small patterns to copy into real applications. They assume
the project is already linked to `texrender::texrender` and that
`texrender_copy_resources(my_target)` defines `TEXRENDER_RES_DIR`.

## Game Or HUD Formula

Keep parsed formulas as `RenderHandle` objects and draw them during your normal
frame loop:

```cpp
TeXRender::LatexDocument document(TEXRENDER_RES_DIR);
auto fps_label = document.render(R"(\mathrm{FPS}=60)", 28.f, 0xffffffff);

while (!WindowShouldClose())
{
    BeginDrawing();
    fps_label.draw(24, 24);
    EndDrawing();
}
```

For raylib and SDL2, destroy documents and handles before closing the graphics
context so cached glyph textures can be released.

## Reusable Formula Label

For application UI code, wrap a parsed `RenderHandle` behind your own label or
widget object instead of reparsing in every frame:

```cpp
class FormulaLabel
{
public:
    FormulaLabel(const TeXRender::LatexDocument& document, std::string source)
        : document_(document)
    {
        set_formula(std::move(source));
    }

    void set_formula(std::string source)
    {
        formula_ = document_.render(source, 42.f, 0xff202840);
    }

    void draw(int x, int baseline_y) const
    {
        TeXRender::draw_at_baseline(*formula_, x, baseline_y);
    }

private:
    const TeXRender::LatexDocument& document_;
    std::optional<TeXRender::RenderHandle> formula_;
};
```

See [`examples/formula_label_widget.cpp`](../examples/formula_label_widget.cpp)
for a complete dependency-shaped example.

## Offline PNG Generation

Use `HeadlessSurface` for command-line tools, CI snapshots, and server-side
preview generation:

```cpp
auto owned = std::make_unique<TeXRender::HeadlessSurface>();
auto* canvas = owned.get();
TeXRender::LatexDocument document(TEXRENDER_RES_DIR, std::move(owned));

auto formula = document.render(R"(\int_0^1 x^2\,dx)", 48.f, 0xff111111);
canvas->resize(formula.width() + 24, formula.height() + 24);
canvas->clear(0xffffffff);
formula.draw(12, 12);
canvas->write_png("formula.png");
```

## SVG Export Pipeline

`SvgSurface` records shapes as SVG primitives and embeds rasterized glyphs as
PNG data URIs:

```cpp
auto owned = std::make_unique<TeXRender::SvgSurface>();
auto* svg = owned.get();
TeXRender::LatexDocument document(TEXRENDER_RES_DIR, std::move(owned));

auto formula = document.render(R"(\sqrt{x+1})", 48.f, 0xff202020);
svg->resize(formula.width() + 24, formula.height() + 24);
svg->clear(0x00000000);
formula.draw(12, 12);
svg->write_svg("formula.svg");
```

This is useful for documentation, reports, generated assets, and web previews
where self-contained files are easier to move around than a resource directory.

## Custom Renderer Adapter

Implement `IDrawSurface` when TeXRender should target a game engine command buffer,
immediate-mode UI draw list, or test double:

```cpp
class MySurface final : public TeXRender::IDrawSurface
{
public:
    void set_color(uint32_t argb) override { color_ = argb; }
    void draw_line(float x1, float y1, float x2, float y2, float width) override;
    void draw_rect(float x, float y, float w, float h, float width) override;
    void fill_rect(float x, float y, float w, float h) override;
    void draw_round_rect(float x, float y, float w, float h, float rx, float ry, float width) override;
    void fill_round_rect(float x, float y, float w, float h, float rx, float ry) override;
    void draw_glyph(const TeXRender::GlyphBitmap& glyph, float pen_x, float pen_y, float angle) override;

private:
    uint32_t color_ = 0xff000000;
};

TeXRender::LatexDocument document(TEXRENDER_RES_DIR, std::make_unique<MySurface>());
```

Copy or upload glyph data during `draw_glyph()`. The borrowed glyph buffer is
valid only for that draw call.

## Safe User Input

Use `try_render()` or `try_render_detailed()` for text typed by users:

```cpp
TeXRender::RenderOptions options;
options.text_size = 42.f;
options.argb = 0xff000000;
options.wrap_width = 480;
options.max_input_bytes = 8192;
options.max_text_size = 96.f;
options.max_wrap_width = 1024;

auto result = document.try_render_detailed(user_text, options);
if (result)
{
    result->draw(20, 20);
}
else
{
    log_validation_error(result.error().message);
}
```

These guardrails are not a sandbox. For internet-scale arbitrary input, keep
request limits in the host application and consider process isolation for
high-risk workloads.

## Portable Feature Checks

Use `texrender/config.hpp` when one codebase supports multiple installed packages:

```cpp
#include <texrender/config.hpp>

static_assert(TeXRender::has_headless);
static_assert(TeXRender::has_svg);

if constexpr (TeXRender::has_sdl2)
{
    // Safe to include/use texrender/sdl2_surface.hpp in this package build.
}
```

From CMake, request required components explicitly:

```cmake
find_package(texrender CONFIG REQUIRED COMPONENTS headless svg)
```

For GUI packages, request `raylib` or `sdl2` only when that installed package was
built with the matching backend.
