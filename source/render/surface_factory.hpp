#pragma once

#include <memory>
#include "texgui/draw_surface.hpp"

namespace TexGui
{
    [[nodiscard]] std::unique_ptr<IDrawSurface> make_surface();
} // namespace TexGui
