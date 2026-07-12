#include <memory>
#include "render/surface_factory.hpp"
#include "texrender/svg_surface.hpp"

std::unique_ptr<TeXRender::IDrawSurface> TeXRender::make_surface()
{
    return std::make_unique<SvgSurface>();
}
