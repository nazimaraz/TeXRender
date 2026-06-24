#include <memory>
#include "render/surface_factory.hpp"
#include "texrender/raylib_surface.hpp"

std::unique_ptr<TeXRender::IDrawSurface> TeXRender::make_surface()
{
    return std::make_unique<RaylibSurface>();
}
