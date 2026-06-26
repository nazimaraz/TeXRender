// Render a formula into SvgSurface and write a self-contained SVG file.
//
//   texrender_svg_export [latex] [out.svg]
#include <cstdio>
#include <memory>
#include <string>
#include <texrender/latex_document.hpp>
#include <texrender/svg_surface.hpp>

#ifndef TEXRENDER_RES_DIR
#   define TEXRENDER_RES_DIR "res"
#endif

int main(const int argc, char** argv)
{
    const auto latex = std::string{argc > 1 ? argv[1] : R"(\sqrt{\frac{a^2+b^2}{c}})"};
    const auto out = std::string{argc > 2 ? argv[2] : "formula.svg"};

    //! [svg-render]
    auto owned = std::make_unique<TeXRender::SvgSurface>();
    auto& canvas = *owned;
    const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};

    const auto formula = document.try_render(latex, 64.f, 0xff1a1a2e);
    if (!formula)
    {
        std::fprintf(stderr, "could not parse: %s\n", latex.c_str());
        return 1;
    }

    constexpr auto margin = 16;
    canvas.resize(formula->width() + 2 * margin, formula->height() + 2 * margin);
    canvas.clear(0xffffffff);
    formula->draw(margin, margin);
    //! [svg-render]

    if (!canvas.write_svg(out))
    {
        std::fprintf(stderr, "could not write: %s\n", out.c_str());
        return 1;
    }

    std::printf("wrote %s (%dx%d, %zu elements)\n", out.c_str(), canvas.width(), canvas.height(), canvas.element_count());
    return 0;
}
