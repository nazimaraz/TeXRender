#include <raylib.h>
#include <texgui/latex_document.hpp>

#ifndef TEXGUI_RES_DIR
#define TEXGUI_RES_DIR "res"
#endif

int main()
{
    InitWindow(900, 520, "texgui raylib overlay");
    SetTargetFPS(60);

    {
        const auto document = TexGui::LatexDocument{TEXGUI_RES_DIR};
        auto formula = document.render(R"(\nabla \cdot \vec{E} = \frac{\rho}{\epsilon_0})", 42.f, 0xff101820);
        auto small = document.render(R"(E = mc^2)", 28.f, 0xfff6f7fb);

        while (!WindowShouldClose())
        {
            BeginDrawing();
            ClearBackground(Color{246, 247, 251, 255});

            DrawRectangle(0, 0, GetScreenWidth(), 72, Color{20, 28, 38, 255});
            DrawText("raylib scene + live texgui overlay", 28, 24, 22, Color{246, 247, 251, 255});
            small.draw(GetScreenWidth() - small.width() - 28, 22);

            DrawCircle(GetScreenWidth() / 2, GetScreenHeight() / 2 + 36, 122, Color{225, 239, 232, 255});
            DrawCircleLines(GetScreenWidth() / 2, GetScreenHeight() / 2 + 36, 122, Color{38, 108, 91, 255});

            DrawRectangleRounded(Rectangle{70.f, 160.f, 500.f, 140.f}, 0.08f, 12, Color{255, 255, 255, 255});
            DrawRectangleLinesEx(Rectangle{70.f, 160.f, 500.f, 140.f}, 2.f, Color{207, 213, 221, 255});
            DrawText("formula rendered by texgui", 96, 186, 20, Color{77, 86, 102, 255});
            formula.draw(96, 224);

            DrawText("press ESC to close", 96, 320, 18, Color{77, 86, 102, 255});
            EndDrawing();
        }
    }

    CloseWindow();
    return 0;
}
