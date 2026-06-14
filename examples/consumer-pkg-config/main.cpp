#include <iostream>
#include <memory>
#include <texrender/headless_surface.hpp>
#include <texrender/latex_document.hpp>

#ifndef TEXRENDER_RES_DIR
#error "Define TEXRENDER_RES_DIR from pkg-config --variable=resdir texrender"
#endif

#ifndef TEXRENDER_EXAMPLE_OUTPUT
#define TEXRENDER_EXAMPLE_OUTPUT "pkg-config-example.png"
#endif

int main()
{
    auto owned = std::make_unique<TeXRender::HeadlessSurface>();
    auto* const canvas = owned.get();
    const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};
    const auto formula = document.try_render(R"(\sum_{n=1}^{\infty}\frac{1}{n^2}=\frac{\pi^2}{6})", 42.f, 0xff202020);
    if (!formula)
        return 1;

    canvas->resize(formula->width() + 24, formula->height() + 24);
    canvas->clear(0xffffffff);
    formula->draw(12, 12);

    if (!canvas->write_png(TEXRENDER_EXAMPLE_OUTPUT))
        return 2;

    std::cout << "wrote " << TEXRENDER_EXAMPLE_OUTPUT << '\n';
    return 0;
}
