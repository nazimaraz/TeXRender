#include <iostream>
#include <memory>
#include <texgui/headless_surface.hpp>
#include <texgui/latex_document.hpp>

#ifndef TEXGUI_RES_DIR
#error "Define TEXGUI_RES_DIR from pkg-config --variable=resdir texgui"
#endif

#ifndef TEXGUI_EXAMPLE_OUTPUT
#define TEXGUI_EXAMPLE_OUTPUT "pkg-config-example.png"
#endif

int main()
{
    auto owned = std::make_unique<TexGui::HeadlessSurface>();
    auto* const canvas = owned.get();
    const auto document = TexGui::LatexDocument{TEXGUI_RES_DIR, std::move(owned)};
    const auto formula = document.try_render(R"(\sum_{n=1}^{\infty}\frac{1}{n^2}=\frac{\pi^2}{6})", 42.f, 0xff202020);
    if (!formula)
        return 1;

    canvas->resize(formula->width() + 24, formula->height() + 24);
    canvas->clear(0xffffffff);
    formula->draw(12, 12);

    if (!canvas->write_png(TEXGUI_EXAMPLE_OUTPUT))
        return 2;

    std::cout << "wrote " << TEXGUI_EXAMPLE_OUTPUT << '\n';
    return 0;
}
