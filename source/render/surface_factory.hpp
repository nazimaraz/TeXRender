#pragma once

#include <memory>
#include "texrender/draw_surface.hpp"

namespace TeXRender
{
    [[nodiscard]] std::unique_ptr<IDrawSurface> make_surface();
} // namespace TeXRender
