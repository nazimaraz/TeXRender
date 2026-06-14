// Minimal custom backend: count MicroTeX draw calls without drawing pixels.
//
// This is useful as a starting point for integrating texrender with an engine that
// has its own command buffer or renderer. Real backends translate these methods
// to their graphics API; the parser, layout, transforms, and glyph rasterization
// stay inside TeXRender.
#include <cstdint>
#include <cstdio>
#include <memory>
#include <cstddef>
#include <texrender/draw_surface.hpp>
#include <texrender/glyph_bitmap.hpp>
#include <texrender/latex_document.hpp>

#ifndef TEXRENDER_RES_DIR
#define TEXRENDER_RES_DIR "res"
#endif

namespace
{
    class CountingSurface final : public TeXRender::IDrawSurface
    {
    public:
        void set_color(uint32_t argb) override
        {
            last_color = argb;
            ++colors;
        }

        void draw_line(float, float, float, float, float) override
        {
            ++lines;
        }

        void draw_rect(float, float, float, float, float) override
        {
            ++rects;
        }

        void fill_rect(float, float, float, float) override
        {
            ++rects;
        }

        void draw_round_rect(float, float, float, float, float, float, float) override
        {
            ++rects;
        }

        void fill_round_rect(float, float, float, float, float, float) override
        {
            ++rects;
        }

        void draw_glyph(const TeXRender::GlyphBitmap& glyph, float, float, float) override
        {
            ++glyphs;
            if (glyph.width > 0 && glyph.height > 0)
                covered_pixels += static_cast<std::size_t>(glyph.width) * static_cast<std::size_t>(glyph.height);
        }

        uint32_t last_color = 0;
        int colors = 0;
        int lines = 0;
        int rects = 0;
        int glyphs = 0;
        std::size_t covered_pixels = 0;
    };
} // namespace

int main()
{
    auto owned = std::make_unique<CountingSurface>();
    auto* const stats = owned.get();
    const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};
    const auto formula = document.render(R"(\frac{a^2+b^2}{c})", 48.f, 0xff223344);

    formula.draw(0, 0);

    std::printf("formula: %dx%d baseline %.1f\n", formula.width(), formula.height(), formula.baseline());
    std::printf("draw calls: colors=%d lines=%d rects=%d glyphs=%d covered=%zu last_color=0x%08x\n", stats->colors, stats->lines,
        stats->rects, stats->glyphs, stats->covered_pixels, stats->last_color);

    return stats->glyphs > 0 ? 0 : 1;
}
