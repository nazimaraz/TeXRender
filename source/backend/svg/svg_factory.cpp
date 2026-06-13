// Compiled only when TEXGUI_BACKEND=svg: documents draw into an SvgSurface
// unless the caller injects a surface of their own.
#include <memory>
#include "render/surface_factory.hpp"
#include "texgui/svg_surface.hpp"

std::unique_ptr<TexGui::IDrawSurface> TexGui::make_surface()
{
    return std::make_unique<SvgSurface>();
}
