#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <texgui/cache_options.hpp>
#include <texgui/formula_layout.hpp>
#include <texgui/headless_surface.hpp>
#include <texgui/latex_document.hpp>
#include <texgui/svg_surface.hpp>
#include <texgui/version.hpp>

#ifndef TEXGUI_RES_DIR
#error "texgui_copy_resources() must define TEXGUI_RES_DIR"
#endif

static_assert(TexGui::version_major == TEXGUI_VERSION_MAJOR);
static_assert(std::is_move_constructible_v<TexGui::RenderHandle>);
static_assert(!std::is_copy_constructible_v<TexGui::RenderHandle>);

int main()
{
    auto limits = TexGui::cache_limits();
    limits.max_cpu_glyphs = 32;
    limits.max_gpu_textures = 32;
    TexGui::set_cache_limits(limits);

    auto owned = std::make_unique<TexGui::HeadlessSurface>();
    auto* const canvas = owned.get();
    const auto document = TexGui::LatexDocument{TEXGUI_RES_DIR, std::move(owned)};
    const auto formula = document.try_render(R"(\sqrt{x+1})", 40.f, 0xff000000);
    if (!formula)
        return 1;

    const auto box = TexGui::formula_box(*formula);
    if (box.width <= 0 || box.height <= 0)
        return 2;

    canvas->resize(formula->width() + 20, formula->height() + 20);
    canvas->clear(0x00000000);
    TexGui::draw_aligned(*formula, 10, 10, TexGui::FormulaAnchor::top);
    if (canvas->covered_pixels() == 0)
        return 3;

    auto svg_owned = std::make_unique<TexGui::SvgSurface>();
    auto* const svg = svg_owned.get();
    const auto svg_document = TexGui::LatexDocument{TEXGUI_RES_DIR, std::move(svg_owned)};
    const auto svg_formula = svg_document.try_render(R"(\frac{1}{2})", 40.f, 0xff223344);
    if (!svg_formula)
        return 4;

    svg->resize(svg_formula->width() + 20, svg_formula->height() + 20);
    svg->clear(0xffffffff);
    svg_formula->draw(10, 10);
    const auto svg_text = svg->svg();
    if (svg_text.find("<svg") == std::string::npos || svg_text.find("data:image/png;base64,") == std::string::npos)
        return 5;

    return 0;
}
