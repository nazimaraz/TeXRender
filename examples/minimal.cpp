#include <raylib.h>
#include "texrender/latex_document.hpp"
#ifndef TEXRENDER_RES_DIR
#   define TEXRENDER_RES_DIR "res"
#endif

int main()
{
    InitWindow(640, 360, "TeXRender");
    SetTargetFPS(60);
    const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR};
    const auto formula = document.render(R"(e^{i\pi} + 1 = 0)", 60.f, 0xff000000);
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        formula.draw((GetScreenWidth() - formula.width()) / 2, (GetScreenHeight() - formula.height()) / 2);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
