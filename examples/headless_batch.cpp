#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <texgui/headless_surface.hpp>
#include <texgui/latex_document.hpp>

#ifndef TEXGUI_RES_DIR
#define TEXGUI_RES_DIR "res"
#endif

int main(int argc, char** argv)
{
    auto formulas = std::vector<std::string>{};
    for (auto i = 1; i < argc; ++i)
        formulas.emplace_back(argv[i]);

    if (formulas.empty())
    {
        formulas = {
            R"(e^{i\pi} + 1 = 0)",
            R"(\sqrt{\frac{a^2+b^2}{c}})",
            R"(\int_0^1 x^2\,dx = \frac{1}{3})",
        };
    }

    auto owned = std::make_unique<TexGui::HeadlessSurface>();
    auto* const canvas = owned.get();
    const auto document = TexGui::LatexDocument{TEXGUI_RES_DIR, std::move(owned)};

    auto index = 1;
    for (const auto& latex : formulas)
    {
        auto options = TexGui::RenderOptions{};
        options.text_size = 48.f;
        options.argb = 0xff111111;
        options.max_input_bytes = 8192;
        options.max_text_size = 128.f;
        options.max_wrap_width = 1200;

        auto formula = document.try_render(latex, options);
        if (!formula)
        {
            std::cerr << "Skipping invalid formula: " << latex << '\n';
            continue;
        }

        constexpr auto margin = 20;
        canvas->resize(formula->width() + 2 * margin, formula->height() + 2 * margin);
        canvas->clear(0xffffffff);
        formula->draw(margin, margin);

        const auto output = "formula-" + std::to_string(index++) + ".png";
        if (!canvas->write_png(output))
        {
            std::cerr << "Could not write " << output << '\n';
            return 1;
        }

        std::cout << output << '\n';
    }

    return 0;
}
