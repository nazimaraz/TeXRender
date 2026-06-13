#include <memory>
#include <optional>
#include <print>
#include <string>
#include "texgui/headless_surface.hpp"
#include "texgui/latex_document.hpp"

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

    for (auto i = 0; i < 50; ++i)
    {
        auto owned = std::make_unique<HeadlessSurface>();
        auto* const canvas = owned.get();
        auto formula = std::optional<RenderHandle>{};

        {
            const auto document = LatexDocument{res, std::move(owned)};
            formula.emplace(document.render(R"(\sum_{n=1}^{10} n^2)", 32.f, 0xff000000));
            CHECK(formula->width() > 0);
            CHECK(formula->height() > 0);
        }

        canvas->resize(formula->width() + 20, formula->height() + 20);
        canvas->clear(0x00000000);
        formula->draw(10, 10);
        CHECK(canvas->covered_pixels() > 0);
        formula.reset();
    }

    for (auto i = 0; i < 100; ++i)
    {
        auto owned = std::make_unique<HeadlessSurface>();
        auto* const canvas = owned.get();
        const auto document = LatexDocument{res, std::move(owned)};
        const auto formula = document.try_render_detailed(R"(\sqrt{x+1})", 28.f, 0xff000000);
        CHECK(formula.has_value());
        if (formula)
        {
            canvas->resize(formula.value().width() + 12, formula.value().height() + 12);
            canvas->clear(0x00000000);
            formula.value().draw(6, 6);
            CHECK(canvas->covered_pixels() > 0);
        }
    }

    if (g_failures == 0)
        std::println("all lifetime_stress tests passed");

    return g_failures == 0 ? 0 : 1;
}
