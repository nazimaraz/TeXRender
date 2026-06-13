// End-to-end render test through the public API: parses real LaTeX, draws it
// into an injected HeadlessSurface, and inspects the resulting pixels. The
// canvas is pure software, so this runs under any backend — no GPU, window, or
// display needed.
#include <memory>
#include <print>
#include <string>
#include "texgui/headless_surface.hpp"
#include "texgui/latex_document.hpp"
#include "texgui/version.hpp"

#ifndef TEXGUI_RES_DIR
#define TEXGUI_RES_DIR "res"
#endif

using namespace TexGui;

namespace
{
    int g_failures = 0;

    void check(const bool ok, const char* const expr)
    {
        if (!ok)
        {
            std::println("FAIL: {}", expr);
            ++g_failures;
        }
    }
} // namespace

#define CHECK(cond) check((cond), #cond)

int main()
{
    const auto res = std::string{TEXGUI_RES_DIR};

    // Compile-coverage for the public version header.
    CHECK(TexGui::version_major == TEXGUI_VERSION_MAJOR);

    // 1) Default surface: a valid formula has positive metrics; try_render never
    //    throws and yields nullopt on malformed input while keeping a value on
    //    valid input.
    {
        const auto document = LatexDocument{res};
        const auto formula = document.render(R"(x^2 + y^2 = z^2)", 40.f, 0xff000000);
        CHECK(formula.width() > 0);
        CHECK(formula.height() > 0);

        CHECK(document.try_render(R"(\sqrt{a})", 40.f, 0xff000000).has_value());
        CHECK(!document.try_render(R"(})", 40.f, 0xff000000).has_value()); // unbalanced '}' -> parse error
    }

    // 2) Full pipeline into an injected headless canvas: drawing a non-empty
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

    // 3) Direct HeadlessSurface compositing: pixels are stored as straight RGBA,
    //    so transparent colors keep their source RGB and compose correctly over
    //    opaque destinations.
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
    }

    if (g_failures == 0)
        std::println("all headless_render tests passed");

    return g_failures == 0 ? 0 : 1;
}
