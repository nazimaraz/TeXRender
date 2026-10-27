#include "texrender/draw_surface.hpp"

using namespace TeXRender;

IDrawSurface::IDrawSurface() noexcept = default;

IDrawSurface::~IDrawSurface() noexcept = default;

float IDrawSurface::glyph_scale() const noexcept
{
    return 1.f;
}
