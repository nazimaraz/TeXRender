#include <iostream>
#include <memory>
#include <texgui/headless_surface.hpp>
#include <texgui/latex_document.hpp>

#ifndef TEXGUI_RES_DIR
#error "texgui_copy_resources() must define TEXGUI_RES_DIR"
#endif

int main()
{
    auto owned = std::make_unique<TexGui::HeadlessSurface>();
    auto* const canvas = owned.get();
    const auto document = TexGui::LatexDocument{TEXGUI_RES_DIR, std::move(owned)};
    const auto formula = document.try_render(R"(\int_0^1 x^2\,dx)", 42.f, 0xff202020);
    if (!formula)
        return 1;

    canvas->resize(formula->width() + 24, formula->height() + 24);
    canvas->clear(0xffffffff);
    formula->draw(12, 12);

    if (!canvas->write_ppm("find-package-example.ppm"))
        return 2;

    std::cout << "wrote find-package-example.ppm\n";
    return 0;
}
