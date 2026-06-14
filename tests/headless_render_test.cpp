// End-to-end render test through the public API: parses real LaTeX, draws it
// into an injected HeadlessSurface, and inspects the resulting pixels. The
// canvas is pure software, so this runs under any backend — no GPU, window, or
// display needed.
#include <exception>
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include "texrender/cache_options.hpp"
#include "texrender/formula_layout.hpp"
#include "texrender/headless_surface.hpp"
#include "texrender/latex_document.hpp"
#include "texrender/svg_surface.hpp"
#include "texrender/version.hpp"

#ifndef TEXRENDER_RES_DIR
#define TEXRENDER_RES_DIR "res"
#endif

using namespace TeXRender;

namespace
{
    int g_failures = 0;

    void check(const bool ok, const char* const expr)
    {
        if (!ok)
        {
            std::printf("FAIL: %s\n", expr);
            ++g_failures;
        }
    }

    void check_render_error(const RenderResult& result, const RenderErrorCode code, const char* const expr)
    {
        if (result.has_value() || result.error().code != code || result.error().message.empty())
        {
            std::printf("FAIL: %s\n", expr);
            ++g_failures;
        }
    }
} // namespace

#define CHECK(cond) check((cond), #cond)
#define CHECK_RENDER_ERROR(result, code) check_render_error((result), (code), #result " -> " #code)

int main()
{
    const auto res = std::string{TEXRENDER_RES_DIR};

    // Compile-coverage for the public version header.
    CHECK(TeXRender::version_major == TEXRENDER_VERSION_MAJOR);

    // 1) Public render path with an injected software surface: a valid formula
    //    has positive metrics; try_render never throws and yields nullopt on
    //    malformed input while keeping a value on valid input.
    {
        const auto document = LatexDocument{res, std::make_unique<HeadlessSurface>()};
        const auto formula = document.render(R"(x^2 + y^2 = z^2)", 40.f, 0xff000000);
        CHECK(formula.width() > 0);
        CHECK(formula.height() > 0);
        CHECK(formula_box(formula).width == formula.width());
        CHECK(top_for_baseline(formula, 100) == static_cast<int>(std::lround(100.f - formula.baseline())));
        CHECK(aligned_top(formula, 100, FormulaAnchor::top) == 100);
        CHECK(aligned_top(formula, 100, FormulaAnchor::baseline) == top_for_baseline(formula, 100));
        CHECK(aligned_top(formula, 100, FormulaAnchor::center) == 100 - formula.height() / 2);
        CHECK(aligned_top(formula, 100, FormulaAnchor::bottom) == 100 - formula.height());

        CHECK(document.try_render(R"(\sqrt{a})", 40.f, 0xff000000).has_value());
        CHECK(!document.try_render(R"(})", 40.f, 0xff000000).has_value()); // unbalanced '}' -> parse error
        CHECK(!document.try_render("x", 600.f, 0xff000000).has_value());

        const auto ok = document.try_render_detailed(R"(\sqrt{a})", 40.f, 0xff000000);
        CHECK(ok.has_value());
        CHECK(ok.error().code == RenderErrorCode::none);

        const auto parse_error = document.try_render_detailed(R"(})", 40.f, 0xff000000);
        CHECK_RENDER_ERROR(parse_error, RenderErrorCode::parse_error);

        const auto text_size_error = document.try_render_detailed("x", 600.f, 0xff000000);
        CHECK_RENDER_ERROR(text_size_error, RenderErrorCode::text_size_too_large);

        auto limited = RenderOptions{};
        limited.text_size = 40.f;
        limited.argb = 0xff000000;
        limited.max_input_bytes = 3;
        CHECK(document.try_render("x", limited).has_value());
        CHECK(!document.try_render("x+x+x", limited).has_value());
        CHECK_RENDER_ERROR(document.try_render_detailed("x+x+x", limited), RenderErrorCode::input_too_large);

        auto invalid_text_size = RenderOptions{};
        invalid_text_size.text_size = 0.f;
        invalid_text_size.argb = 0xff000000;
        CHECK_RENDER_ERROR(document.try_render_detailed("x", invalid_text_size), RenderErrorCode::invalid_options);

        auto invalid_wrap = RenderOptions{};
        invalid_wrap.text_size = 40.f;
        invalid_wrap.argb = 0xff000000;
        invalid_wrap.wrap_width = -1;
        CHECK(!document.try_render("x", invalid_wrap).has_value());
        CHECK_RENDER_ERROR(document.try_render_detailed("x", invalid_wrap), RenderErrorCode::invalid_options);

        auto invalid_wrap_limit = RenderOptions{};
        invalid_wrap_limit.text_size = 40.f;
        invalid_wrap_limit.argb = 0xff000000;
        invalid_wrap_limit.max_wrap_width = -1;
        CHECK_RENDER_ERROR(document.try_render_detailed("x", invalid_wrap_limit), RenderErrorCode::invalid_options);

        auto excessive_wrap = RenderOptions{};
        excessive_wrap.text_size = 40.f;
        excessive_wrap.argb = 0xff000000;
        excessive_wrap.wrap_width = 120;
        excessive_wrap.max_wrap_width = 80;
        CHECK_RENDER_ERROR(document.try_render_detailed("x", excessive_wrap), RenderErrorCode::wrap_width_too_large);

        constexpr auto long_formula = R"(x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x+x)";
        const auto unwrapped = document.render(long_formula, 32.f, 0xff000000);
        auto constrained_width = RenderOptions{};
        constrained_width.text_size = 32.f;
        constrained_width.argb = 0xff000000;
        constrained_width.max_wrap_width = 80;
        const auto wrapped_by_limit = document.render(long_formula, constrained_width);
        CHECK(wrapped_by_limit.width() <= unwrapped.width());
        CHECK(wrapped_by_limit.height() >= unwrapped.height());

        auto threw = false;
        try
        {
            (void) document.render("x+x+x", limited);
        } catch (const std::exception&)
        {
            threw = true;
        }
        CHECK(threw);
    }

    // 2) RenderHandle is move-only and keeps the document state alive after the
    //    LatexDocument object itself is destroyed.
    {
        auto surviving = std::optional<RenderHandle>{};
        auto* canvas = static_cast<HeadlessSurface*>(nullptr);
        {
            auto owned = std::make_unique<HeadlessSurface>();
            canvas = owned.get();
            const auto document = LatexDocument{res, std::move(owned)};
            auto formula = document.render(R"(\int_0^1 x^2\,dx)", 42.f, 0xff111111);
            CHECK(formula.width() > 0);
            surviving = std::move(formula);
        }

        CHECK(surviving.has_value());
        canvas->resize(surviving->width() + 24, surviving->height() + 24);
        canvas->clear(0x00000000);
        surviving->draw(12, 12);
        CHECK(canvas->covered_pixels() > 0);
        surviving.reset();
    }

    // 3) Full pipeline into an injected headless canvas: drawing a non-empty
    //    formula must leave visible pixels; drawing nothing must leave the
    //    canvas blank.
    {
        auto owned = std::make_unique<HeadlessSurface>();
        auto* const canvas = owned.get();
        const auto document = LatexDocument{res, std::move(owned)};
        CHECK(&document.surface() == canvas); // the injected surface is the one in use

        const auto formula = document.render(R"(\sqrt{\frac{a^2+b^2}{c}})", 48.f, 0xff000000);
        CHECK(formula.width() > 0);

        canvas->resize(formula.width() + 40, formula.height() + 40);
        canvas->clear(0x00000000);
        CHECK(canvas->covered_pixels() == 0); // blank after clear

        formula.draw(20, 20);
        CHECK(canvas->covered_pixels() > 0); // the formula rendered something
    }

    // 4) Repeated render/draw cycles remain valid under intentionally tiny cache
    //    limits. This exercises cache eviction without depending on internals.
    {
        const auto old_limits = cache_limits();
        set_cache_limits(CacheLimits{1, 1});

        auto owned = std::make_unique<HeadlessSurface>();
        auto* const canvas = owned.get();
        const auto document = LatexDocument{res, std::move(owned)};
        for (const auto* const latex : {R"(x)", R"(\sqrt{x+1})", R"(\frac{a}{b})", R"(\hat{x}+\vec{v})"})
        {
            auto formula = document.render(latex, 38.f, 0xff000000);
            canvas->resize(formula.width() + 16, formula.height() + 16);
            canvas->clear(0x00000000);
            formula.draw(8, 8);
            CHECK(canvas->covered_pixels() > 0);
        }

        set_cache_limits(old_limits);
    }

    // 5) Direct HeadlessSurface compositing: pixels are stored as straight RGBA,
    //    so transparent colors keep their source RGB and compose correctly over
    //    opaque destinations. Non-finite or extreme direct draw inputs are
    //    ignored or clipped instead of reaching undefined float-to-int casts.
    {
        auto canvas = HeadlessSurface{};
        canvas.resize(1, 1);
        canvas.clear(0x00000000);
        canvas.set_color(0x80ff0000);
        canvas.fill_rect(0, 0, 1, 1);
        const auto& transparent = canvas.pixels();
        CHECK(transparent[0] == 255);
        CHECK(transparent[1] == 0);
        CHECK(transparent[2] == 0);
        CHECK(transparent[3] == 128);

        canvas.clear(0xffffffff);
        canvas.set_color(0x800000ff);
        canvas.fill_rect(0, 0, 1, 1);
        const auto& opaque = canvas.pixels();
        CHECK(opaque[0] == 127);
        CHECK(opaque[1] == 127);
        CHECK(opaque[2] == 255);
        CHECK(opaque[3] == 255);

        canvas.resize(8, 8);
        canvas.clear(0x00000000);
        canvas.set_color(0xff00ff00);
        canvas.fill_rect(std::numeric_limits<float>::quiet_NaN(), 0.f, 2.f, 2.f);
        canvas.fill_rect(0.f, std::numeric_limits<float>::infinity(), 2.f, 2.f);
        CHECK(canvas.covered_pixels() == 0);

        canvas.draw_line(-1.0e20f, 4.f, 1.0e20f, 4.f, 1.f);
        CHECK(canvas.covered_pixels() > 0);

        const unsigned char glyph_pixels[4] = {255, 255, 255, 255};
        const auto before_invalid_glyph = canvas.covered_pixels();
        canvas.draw_glyph(GlyphBitmap{glyph_pixels, 2, 2, -2, 0, 0, 1}, 0.f, 0.f, 0.f);
        canvas.draw_glyph(GlyphBitmap{glyph_pixels, 2, 2, 2, 0, 0, 2}, std::numeric_limits<float>::infinity(), 0.f, 0.f);
        CHECK(canvas.covered_pixels() == before_invalid_glyph);
    }

    // 6) Cache limits are configurable and clamp zero to one.
    {
        const auto old_limits = cache_limits();
        auto limits = old_limits;
        limits.max_cpu_glyphs = 0;
        limits.max_gpu_textures = 0;
        set_cache_limits(limits);
        CHECK(cache_limits().max_cpu_glyphs == 1);
        CHECK(cache_limits().max_gpu_textures == 1);
        set_cache_limits(old_limits);
    }

    // 7) HeadlessSurface can write real PNG files without extra dependencies.
    {
        auto canvas = HeadlessSurface{};
        CHECK(!canvas.write_png((std::filesystem::temp_directory_path() / "texrender-empty.png").string()));

        canvas.resize(2, 1);
        canvas.clear(0x00000000);
        canvas.set_color(0xffff0000);
        canvas.fill_rect(0, 0, 1, 1);

        const auto path = std::filesystem::temp_directory_path() / "texrender-headless-surface-test.png";
        CHECK(canvas.write_png(path.string()));

        auto file = std::ifstream{path, std::ios::binary};
        auto signature = std::array<unsigned char, 8>{};
        file.read(reinterpret_cast<char*>(signature.data()), static_cast<std::streamsize>(signature.size()));
        const auto expected = std::array<unsigned char, 8>{0x89u, 0x50u, 0x4eu, 0x47u, 0x0du, 0x0au, 0x1au, 0x0au};
        CHECK(signature == expected);
        std::filesystem::remove(path);
    }

    // 8) SvgSurface can capture a formula as a self-contained SVG document.
    {
        auto owned = std::make_unique<SvgSurface>();
        auto* const canvas = owned.get();
        const auto document = LatexDocument{res, std::move(owned)};
        const auto formula = document.render(R"(\sqrt{x+1})", 40.f, 0xff224466);
        canvas->resize(formula.width() + 20, formula.height() + 20);
        canvas->clear(0xffffffff);
        formula.draw(10, 10);

        const auto svg = canvas->svg();
        CHECK(canvas->element_count() > 1);
        CHECK(svg.find("<svg") != std::string::npos);
        CHECK(svg.find("<image") != std::string::npos);
        CHECK(svg.find("data:image/png;base64,") != std::string::npos);

        const auto path = std::filesystem::temp_directory_path() / "texrender-svg-surface-test.svg";
        CHECK(canvas->write_svg(path.string()));
        auto file = std::ifstream{path};
        auto first_line = std::string{};
        std::getline(file, first_line);
        CHECK(first_line.find("<svg") != std::string::npos);
        std::filesystem::remove(path);
    }

    if (g_failures == 0)
        std::printf("all headless_render tests passed\n");

    return g_failures == 0 ? 0 : 1;
}
