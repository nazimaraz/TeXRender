// Compiled only when TEXGUI_BACKEND=raylib: documents draw into a
// RaylibSurface unless the caller injects a surface of their own.
#include <memory>
#include "render/surface_factory.hpp"
#include "texgui/raylib_surface.hpp"

std::unique_ptr<TexGui::IDrawSurface> TexGui::make_surface()
{
    return std::make_unique<RaylibSurface>();
}
