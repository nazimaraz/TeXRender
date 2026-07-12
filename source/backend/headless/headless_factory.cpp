#include <memory>
#include "render/surface_factory.hpp"
#include "texrender/headless_surface.hpp"

std::unique_ptr<TeXRender::IDrawSurface> TeXRender::make_surface()
{
    return std::make_unique<HeadlessSurface>();
}
