#include "render/surface_factory.hpp"
#include "texrender/imgui_surface.hpp"

namespace TeXRender
{
    std::unique_ptr<IDrawSurface> make_surface()
    {
        return std::make_unique<ImGuiSurface>();
    }
} // namespace TeXRender
