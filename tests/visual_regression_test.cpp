#include <algorithm>
#include <cstddef>
#include <memory>
#include <print>
#include <string>
#include <vector>
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

    struct Bounds
    {
        int min_x = 0;
        int min_y = 0;
        int max_x = -1;
        int max_y = -1;
    };

    [[nodiscard]] Bounds ink_bounds(const HeadlessSurface& canvas)
    {
        auto bounds = Bounds{};
        auto first = true;
        const auto& pixels = canvas.pixels();
        for (auto y = 0; y < canvas.height(); ++y)
        {
            for (auto x = 0; x < canvas.width(); ++x)
            {
                const auto idx =
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(canvas.width()) + static_cast<std::size_t>(x)) * 4u;
                if (pixels[idx + 3] == 0)
                    continue;

                if (first)
                {
                    bounds = {x, y, x, y};
                    first = false;
                }
                else
                {
                    bounds.min_x = std::min(bounds.min_x, x);
                    bounds.min_y = std::min(bounds.min_y, y);
                    bounds.max_x = std::max(bounds.max_x, x);
                    bounds.max_y = std::max(bounds.max_y, y);
                }
            }
        }

        return bounds;
    }

    [[nodiscard]] bool has_horizontal_rule(const HeadlessSurface& canvas, const int min_run)
    {
        const auto& pixels = canvas.pixels();
        for (auto y = 0; y < canvas.height(); ++y)
        {
            auto run = 0;
            for (auto x = 0; x < canvas.width(); ++x)
            {
                const auto idx =
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(canvas.width()) + static_cast<std::size_t>(x)) * 4u;
                run = pixels[idx + 3] != 0 ? run + 1 : 0;
                if (run >= min_run)
                    return true;
            }
        }

        return false;
    }
} // namespace

#define CHECK(cond) check((cond), #cond)

int main()
{
    auto owned = std::make_unique<HeadlessSurface>();
    auto* const canvas = owned.get();
    const auto document = LatexDocument{TEXGUI_RES_DIR, std::move(owned)};
    const auto formula = document.render(R"(\frac{a+b}{c+d})", 56.f, 0xff000000);

    constexpr auto margin = 12;
    canvas->resize(formula.width() + 2 * margin, formula.height() + 2 * margin);
    canvas->clear(0x00000000);
    formula.draw(margin, margin);

    const auto bounds = ink_bounds(*canvas);
    CHECK(canvas->covered_pixels() > 150);
    CHECK(bounds.min_x >= margin - 2);
    CHECK(bounds.min_y >= margin - 2);
    CHECK(bounds.max_x < canvas->width() - margin + 2);
    CHECK(bounds.max_y < canvas->height() - margin + 2);
    CHECK(bounds.max_x - bounds.min_x > 30);
    CHECK(bounds.max_y - bounds.min_y > 25);
    CHECK(has_horizontal_rule(*canvas, std::max(12, formula.width() / 3)));

    if (g_failures == 0)
        std::println("all visual_regression tests passed");

    return g_failures == 0 ? 0 : 1;
}
