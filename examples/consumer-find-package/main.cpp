#include <iostream>
#include <memory>
#include <texrender/headless_surface.hpp>
#include <texrender/latex_document.hpp>

#ifndef TEXRENDER_RES_DIR
#error "texrender_copy_resources() must define TEXRENDER_RES_DIR"
#endif

int main()
{
    auto owned = std::make_unique<TeXRender::HeadlessSurface>();
    auto* const canvas = owned.get();
    const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};
    const auto formula = document.try_render(R"(\int_0^1 x^2\,dx)", 42.f, 0xff202020);
    if (!formula)
        return 1;

    canvas->resize(formula->width() + 24, formula->height() + 24);
    canvas->clear(0xffffffff);
    formula->draw(12, 12);

    if (!canvas->write_png("find-package-example.png"))
        return 2;

    std::cout << "wrote find-package-example.png\n";
    return 0;
}
