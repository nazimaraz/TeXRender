#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <texrender/cache_options.hpp>
#include <texrender/formula_layout.hpp>
#include <texrender/headless_surface.hpp>
#include <texrender/latex_document.hpp>
#include <texrender/svg_surface.hpp>
#include <texrender/version.hpp>

#ifndef TEXRENDER_RES_DIR
#error "texrender_copy_resources() must define TEXRENDER_RES_DIR"
#endif

static_assert(TeXRender::version_major == TEXRENDER_VERSION_MAJOR);
static_assert(std::is_move_constructible_v<TeXRender::RenderHandle>);
static_assert(!std::is_copy_constructible_v<TeXRender::RenderHandle>);

int main()
{
    auto limits = TeXRender::cache_limits();
    limits.max_cpu_glyphs = 32;
    limits.max_gpu_textures = 32;
    TeXRender::set_cache_limits(limits);

    auto owned = std::make_unique<TeXRender::HeadlessSurface>();
    auto* const canvas = owned.get();
    const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};
    const auto formula = document.try_render(R"(\sqrt{x+1})", 40.f, 0xff000000);
    if (!formula)
        return 1;

    const auto box = TeXRender::formula_box(*formula);
    if (box.width <= 0 || box.height <= 0)
        return 2;

    canvas->resize(formula->width() + 20, formula->height() + 20);
    canvas->clear(0x00000000);
    TeXRender::draw_aligned(*formula, 10, 10, TeXRender::FormulaAnchor::top);
    if (canvas->covered_pixels() == 0)
        return 3;

    auto svg_owned = std::make_unique<TeXRender::SvgSurface>();
    auto* const svg = svg_owned.get();
    const auto svg_document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(svg_owned)};
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
