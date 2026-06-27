#include "test_helpers.hpp"
#include <initializer_list>
#include <memory>
#include <string_view>
#include <vector>
#include "texrender/headless_surface.hpp"
#include "texrender/latex_document.hpp"

namespace
{
    [[nodiscard]] bool has_red_pixel(const std::vector<unsigned char>& pixels)
    {
        for (auto index = std::size_t{0}; index + 3 < pixels.size(); index += 4)
        {
            if (pixels[index + 0] == 255u && pixels[index + 1] == 0u && pixels[index + 2] == 0u && pixels[index + 3] > 0u)
                return true;
        }

        return false;
    }

    [[nodiscard]] bool recolors_render_handle(const TeXRender::LatexDocument& document, TeXRender::HeadlessSurface& canvas)
    {
        auto result = document.try_render_detailed(R"(E = mc^2)", 40.f, 0xff000000, 512);
        if (!result)
            return false;

        constexpr auto margin = 8;
        canvas.resize(result->width() + 2 * margin, result->height() + 2 * margin);
        canvas.clear(0x00000000);
        result->draw(margin, margin);
        const auto black_pixels = canvas.pixels();

        result->set_foreground(0xffff0000);
        canvas.clear(0x00000000);
        result->draw(margin, margin);
        const auto red_pixels = canvas.pixels();

        return black_pixels != red_pixels && has_red_pixel(red_pixels);
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
    for (const auto& [name, latex] : valid_cases)
        ok = TeXRender::Test::run_case(name, TeXRender::Test::render_non_empty(document, *canvas, latex)) && ok;

    ok = TeXRender::Test::run_case("render-handle-recolors", recolors_render_handle(document, *canvas)) && ok;
    return ok ? 0 : 1;
}
