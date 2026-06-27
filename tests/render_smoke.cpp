#include <algorithm>
#include <array>
#include <cstdio>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "texrender/headless_surface.hpp"
#include "texrender/latex_document.hpp"
#include "texrender/svg_surface.hpp"

namespace
{
    [[nodiscard]] bool render_non_empty(const TeXRender::LatexDocument& document, TeXRender::HeadlessSurface& canvas,
        const std::string_view latex)
    {
        auto result = document.try_render_detailed(std::string{latex}, 36.f, 0xff000000, 512);
        if (!result)
        {
            std::fprintf(stderr, "render failed: %s\n", result.error().message.c_str());
            return false;
        }

        constexpr auto margin = 8;
        canvas.resize(result->width() + 2 * margin, result->height() + 2 * margin);
        canvas.clear(0x00000000);
        result->draw(margin, margin);

        if (result->width() <= 0 || result->height() <= 0 || canvas.covered_pixels() == 0)
        {
            std::fprintf(stderr, "empty render for: %.*s\n", static_cast<int>(latex.size()), latex.data());
            return false;
        }

        return true;
    }

    [[nodiscard]] bool expect_render_error(const TeXRender::LatexDocument& document, const std::string_view latex,
        const TeXRender::RenderOptions& options, const TeXRender::RenderErrorCode expected_code)
    {
        if (const auto result = document.try_render_detailed(std::string{latex}, options);
            result || result.error().code != expected_code)
        {
            std::fprintf(stderr, "expected render error %u for: %.*s\n", static_cast<unsigned>(expected_code),
                static_cast<int>(latex.size()), latex.data());
            return false;
        }

        return true;
    }

    [[nodiscard]] bool run_case(const char* name, const bool passed)
    {
        if (passed)
            return true;

        std::fprintf(stderr, "case failed: %s\n", name);
        return false;
    }

    [[nodiscard]] std::size_t count_occurrences(const std::string& value, const std::string& needle)
    {
        auto count = std::size_t{0};
        auto offset = std::size_t{0};
        while ((offset = value.find(needle, offset)) != std::string::npos)
        {
            ++count;
            offset += needle.size();
        }

        return count;
    }

    [[nodiscard]] bool svg_reuses_glyph_defs()
    {
        constexpr auto pixels = std::array<unsigned char, 4>{255u, 128u, 128u, 255u};
        const auto glyph = TeXRender::GlyphBitmap{pixels.data(), 2, 2, 2, 0, 0, 42};
        auto surface = TeXRender::SvgSurface{16, 16};
        surface.set_color(0xff000000);
        surface.draw_glyph(glyph, 1.f, 8.f, 0.f);
        surface.draw_glyph(glyph, 5.f, 8.f, 0.f);
        const auto svg = surface.svg();
        return svg.find("<defs>") != std::string::npos && count_occurrences(svg, "data:image/png;base64,") == 1 &&
               count_occurrences(svg, "<use href=\"#tr-glyph-0\"") == 2;
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
} // namespace

int main()
{
    auto owned = std::make_unique<TeXRender::HeadlessSurface>();
    auto* const canvas = owned.get();
    const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};
    const auto valid_cases = std::initializer_list<std::pair<const char*, std::string_view>>{
        {"integral", R"(\int_0^\infty e^{-x^2}\,dx=\frac{\sqrt{\pi}}{2})"},
        {"fraction-sum", R"(\frac{a+b}{\sqrt{c}}+\sum_{n=1}^{5} n^2)"},
        {"physics", R"(\mathrm{Energy}=mc^2)"},
        {"text", R"(\text{OpenMath smoke test})"},
    };
    auto ok = true;
    ok = run_case("svg-glyph-def-reuse", svg_reuses_glyph_defs()) && ok;
    ok = run_case("headless-blend-common-paths", headless_blends_common_paths()) && ok;
    for (const auto& [name, latex] : valid_cases)
        ok = run_case(name, render_non_empty(document, *canvas, latex)) && ok;

    auto options = TeXRender::RenderOptions{};
    options.text_size = 36.f;
    options.argb = 0xff000000;
    options.wrap_width = 512;
    options.max_input_bytes = 16;
    options.max_text_size = 0.f;
    options.max_wrap_width = 0;
    ok = run_case("parse-error", expect_render_error(document, R"(a\\b)", options, TeXRender::RenderErrorCode::parse_error)) && ok;
    options = TeXRender::RenderOptions{};
    options.text_size = 600.f;
    options.max_text_size = 512.f;
    ok = run_case("text-size-limit",
             expect_render_error(document, R"(\text{too large})", options, TeXRender::RenderErrorCode::text_size_too_large)) &&
         ok;
    options = TeXRender::RenderOptions{};
    options.text_size = 36.f;
    options.max_input_bytes = 4;
    ok = run_case("input-limit",
             expect_render_error(document, R"(\frac{1}{2})", options, TeXRender::RenderErrorCode::input_too_large)) &&
         ok;
    options = TeXRender::RenderOptions{};
    options.text_size = 36.f;
    options.wrap_width = 100000;
    options.max_wrap_width = 4096;
    ok = run_case("wrap-limit",
             expect_render_error(document, R"(\text{wrap})", options, TeXRender::RenderErrorCode::wrap_width_too_large)) &&
         ok;
    return ok ? 0 : 1;
}
