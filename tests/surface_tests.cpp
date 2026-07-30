#include "test_helpers.hpp"
#include <array>
#include <vector>
#include "texrender/glyph_bitmap.hpp"
#include "texrender/headless_surface.hpp"
#include "texrender/svg_surface.hpp"

namespace
{
    [[nodiscard]] bool svg_reuses_glyph_defs()
    {
        constexpr auto pixels = std::array<unsigned char, 4>{255u, 128u, 128u, 255u};
        const auto glyph = TeXRender::GlyphBitmap{pixels.data(), 2, 2, 2, 0, 0, 42};
        auto surface = TeXRender::SvgSurface{16, 16};
        surface.set_color(0xff000000);
        surface.draw_glyph(glyph, 1.f, 8.f, 0.f);
        surface.draw_glyph(glyph, 5.f, 8.f, 0.f);
        const auto svg = surface.svg();
        return svg.find("<defs>") != std::string::npos && TeXRender::Test::count_occurrences(svg, "data:image/png;base64,") == 1 &&
               TeXRender::Test::count_occurrences(svg, "<use href=\"#tr-glyph-0\"") == 2;
    }

    [[nodiscard]] bool svg_tracks_shape_elements()
    {
        auto surface = TeXRender::SvgSurface{32, 24};
        surface.clear(0xffffffff);
        surface.set_color(0xff123456);
        surface.draw_line(1.f, 2.f, 20.f, 2.f, 2.f);
        surface.fill_rect(4.f, 5.f, 6.f, 7.f);
        const auto svg = surface.svg();
        return surface.width() == 32 && surface.height() == 24 && surface.element_count() == 3 &&
               svg.find("<svg") != std::string::npos && svg.find("<line") != std::string::npos &&
               svg.find("<rect") != std::string::npos;
    }

    [[nodiscard]] bool headless_blends_common_paths()
    {
        auto surface = TeXRender::HeadlessSurface{};
        surface.resize(1, 1);

        constexpr auto opaque = std::array<unsigned char, 1>{255u};
        const auto opaque_glyph = TeXRender::GlyphBitmap{opaque.data(), 1, 1, 1, 0, 0, 1};
        surface.clear(0x00000000);
        surface.set_color(0x80ff0000);
        surface.draw_glyph(opaque_glyph, 0.f, 0.f, 0.f);
        if (surface.pixels() != std::vector<unsigned char>{255u, 0u, 0u, 128u})
            return false;

        constexpr auto half = std::array<unsigned char, 1>{128u};
        const auto half_glyph = TeXRender::GlyphBitmap{half.data(), 1, 1, 1, 0, 0, 2};
        surface.clear(0x00000000);
        surface.set_color(0xff00ff00);
        surface.draw_glyph(half_glyph, 0.f, 0.f, 0.f);
        if (surface.pixels() != std::vector<unsigned char>{0u, 255u, 0u, 128u})
            return false;

        surface.clear(0xffffffff);
        surface.set_color(0x80000000);
        surface.fill_rect(0.f, 0.f, 1.f, 1.f);
        if (surface.pixels() != std::vector<unsigned char>{127u, 127u, 127u, 255u})
            return false;

        surface.set_color(0xff0000ff);
        surface.draw_glyph(opaque_glyph, 0.f, 0.f, 0.f);
        return surface.pixels() == std::vector<unsigned char>{0u, 0u, 255u, 255u};
    }

    [[nodiscard]] bool headless_clips_and_counts_pixels()
    {
        auto surface = TeXRender::HeadlessSurface{};
        surface.resize(4, 4);
        surface.clear(0x00000000);
        surface.set_color(0xff000000);
        surface.fill_rect(-2.f, -2.f, 4.f, 4.f);
        return surface.width() == 4 && surface.height() == 4 && surface.covered_pixels() == 4;
    }
} // namespace

int main()
{
    auto ok = true;
    ok = TeXRender::Test::run_case("svg-glyph-def-reuse", svg_reuses_glyph_defs()) && ok;
    ok = TeXRender::Test::run_case("svg-tracks-shape-elements", svg_tracks_shape_elements()) && ok;
    ok = TeXRender::Test::run_case("headless-blend-common-paths", headless_blends_common_paths()) && ok;
    ok = TeXRender::Test::run_case("headless-clips-and-counts-pixels", headless_clips_and_counts_pixels()) && ok;
    return ok ? 0 : 1;
}
