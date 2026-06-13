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
    const auto formula = document.try_render(R"(\sqrt{x+1})", 40.f, 0xff000000);
    if (!formula)
        return 1;

    canvas->resize(formula->width() + 20, formula->height() + 20);
    canvas->clear(0x00000000);
    formula->draw(10, 10);
    return canvas->covered_pixels() > 0 ? 0 : 2;
}
