#include "render/surface_factory.hpp"
#include "texrender/sdl2_surface.hpp"

namespace TeXRender
{
    std::unique_ptr<IDrawSurface> make_surface()
    {
        return std::make_unique<Sdl2Surface>();
    }
} // namespace TeXRender
