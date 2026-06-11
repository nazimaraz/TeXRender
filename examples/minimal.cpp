#include <raylib.h>
#include "texgui/latex_document.hpp"

#ifndef TEXGUI_RES_DIR
#define TEXGUI_RES_DIR "res"
#endif

int main()
{
    InitWindow(640, 360, "latex-renderer");
    SetTargetFPS(60);
    // Scope the document/handle so they are destroyed (freeing GPU glyph textures)
    // while the window is still alive, before CloseWindow().
    {
        const auto document = TexGui::LatexDocument{TEXGUI_RES_DIR};
        const auto formula = document.render(R"(e^{i\pi} + 1 = 0)", 60.f, 0xff000000);
        while (!WindowShouldClose())
        {
            BeginDrawing();
            ClearBackground(RAYWHITE);
            formula.draw((GetScreenWidth() - formula.width()) / 2, (GetScreenHeight() - formula.height()) / 2);
            EndDrawing();
        }
    }

    CloseWindow();
    return 0;
}
