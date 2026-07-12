#include <array>
#include <cstdlib>
#include <raylib.h>
#include "texrender/latex_document.hpp"
#ifndef TEXRENDER_RES_DIR
#   define TEXRENDER_RES_DIR "res"
#endif

namespace
{
    constexpr auto k_samples = std::array{
        R"(\sqrt{\frac{a^2+b^2}{c}} = \sum_{i=1}^{n} x_i)",
        R"(e^{i\pi} + 1 = 0)",
        R"(\int_{0}^{\infty} e^{-x^2}\,dx = \frac{\sqrt{\pi}}{2})",
        R"(\frac{\partial}{\partial t}\,\Psi = \hat{H}\,\Psi)",
        R"(\begin{pmatrix} a & b \\ c & d \end{pmatrix}\begin{pmatrix} x \\ y \end{pmatrix})",
    };
    constexpr auto k_background = Color{250, 250, 250, 255};
    constexpr auto k_foreground = std::uint32_t{0xff1a1a2e};

    [[nodiscard]] TeXRender::RenderHandle buildFormula(const TeXRender::LatexDocument& document, const std::size_t index,
        const float text_size)
    {
        return document.render(k_samples.at(index), text_size, k_foreground);
    }
} // namespace

int main()
{
    InitWindow(900, 500, "TeXRender (MicroTeX + raylib)");
    SetTargetFPS(60);
    const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR};
    auto index = std::size_t{0};
    auto text_size = 50.f;
    auto formula = buildFormula(document, index, text_size);
    auto dirty = false;
    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_RIGHT))
        {
            index = (index + 1) % k_samples.size();
            dirty = true;
        }

        if (IsKeyPressed(KEY_LEFT))
        {
            index = (index + k_samples.size() - 1) % k_samples.size();
            dirty = true;
        }

        if (IsKeyPressed(KEY_UP))
        {
            text_size = text_size + 4.f > 160.f ? 160.f : text_size + 4.f;
            dirty = true;
        }

        if (IsKeyPressed(KEY_DOWN))
        {
            text_size = text_size - 4.f < 12.f ? 12.f : text_size - 4.f;
            dirty = true;
        }

        if (dirty)
        {
            formula = buildFormula(document, index, text_size);
            dirty = false;
        }

        BeginDrawing();
        ClearBackground(k_background);
        const auto x = std::max(0, (GetScreenWidth() - formula.width()) / 2);
        const auto y = std::max(0, (GetScreenHeight() - formula.height()) / 2);
        formula.draw(x, y);
        DrawText("SPACE/<- ->: cycle    UP/DOWN: size    ESC: quit", 16, 16, 18, Color{120, 120, 130, 255});
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
