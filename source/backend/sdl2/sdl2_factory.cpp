#include "render/surface_factory.hpp"
#include "texgui/sdl2_surface.hpp"

namespace TexGui
{
    std::unique_ptr<IDrawSurface> make_surface()
    {
        return std::make_unique<Sdl2Surface>();
    }
} // namespace TexGui
