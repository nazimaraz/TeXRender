#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include "texrender/headless_surface.hpp"
#include "texrender/latex_document.hpp"

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

        const auto margin = 8;
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
} // namespace

int main()
{
    auto owned = std::make_unique<TeXRender::HeadlessSurface>();
    auto* const canvas = owned.get();
    const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};

    const auto formulas = {
        R"(\int_0^\infty e^{-x^2}\,dx=\frac{\sqrt{\pi}}{2})",
        R"(\frac{a+b}{\sqrt{c}}+\sum_{n=1}^{5} n^2)",
        R"(\mathrm{Energy}=mc^2)",
        R"(\text{OpenMath smoke test})",
    };

    return std::all_of(formulas.begin(), formulas.end(),
               [&](const std::string_view latex) {
                   return render_non_empty(document, *canvas, latex);
               })
               ? 0
               : 1;
}
