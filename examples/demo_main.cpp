#include <array>
#include <cstdlib>
#include <optional>
#include <raylib.h>
#include "texrender/latex_document.hpp"

#ifndef TEXRENDER_RES_DIR
#define TEXRENDER_RES_DIR "res"
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
} // namespace

int main()
{
    InitWindow(900, 500, "TeXRender (MicroTeX + raylib)");
    SetTargetFPS(60);

    {
        const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR};
        auto index = std::size_t{0};
        if (const auto* const start = std::getenv("TEXRENDER_SAMPLE"))
        {
            char* end{};
            if (const auto value = std::strtol(start, &end, 10); end != start && value >= 0)
                index = static_cast<std::size_t>(value) % k_samples.size();
        }

        auto text_size = 50.f;
        auto formula = std::optional<TeXRender::RenderHandle>{};
        const auto rebuild = [&] {
            formula = document.render(k_samples[index], text_size, k_foreground);
        };
        rebuild();
        const auto draw_scene = [&] {
            ClearBackground(k_background);
            const auto x = std::max(0, (GetScreenWidth() - formula->width()) / 2);
            const auto y = std::max(0, (GetScreenHeight() - formula->height()) / 2);
            formula->draw(x, y);
            DrawText("SPACE/<- ->: cycle    UP/DOWN: size    ESC: quit", 16, 16, 18, Color{120, 120, 130, 255});
        };
        if (const auto* const shot = std::getenv("TEXRENDER_SCREENSHOT"))
        {
            const auto target = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
            BeginTextureMode(target);
            draw_scene();
            EndTextureMode();
            auto image = LoadImageFromTexture(target.texture);
            ImageFlipVertical(&image);
            ExportImage(image, shot);
            UnloadImage(image);
            UnloadRenderTexture(target);
        }
        else
        {
            while (!WindowShouldClose())
            {
                auto dirty = false;
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
                    rebuild();

                BeginDrawing();
                draw_scene();
                EndDrawing();
            }
        }
    }

    CloseWindow();
    return 0;
}
