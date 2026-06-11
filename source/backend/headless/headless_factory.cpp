// Compiled only when TEXGUI_BACKEND=headless: documents draw into an
// in-memory HeadlessSurface unless the caller injects a surface of their own.
#include <memory>
#include "render/surface_factory.hpp"
#include "texgui/headless_surface.hpp"

std::unique_ptr<TexGui::IDrawSurface> TexGui::make_surface()
{
    return std::make_unique<HeadlessSurface>();
}
