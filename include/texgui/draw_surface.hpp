#pragma once

#include <cstdint>
#include "texgui/glyph_bitmap.hpp"

namespace TexGui
{
    class IDrawSurface
    {
    public:
        IDrawSurface();
        IDrawSurface(const IDrawSurface&) = delete;
        IDrawSurface(IDrawSurface&&) = delete;
        IDrawSurface& operator=(const IDrawSurface&) = delete;
        IDrawSurface& operator=(IDrawSurface&&) = delete;
        virtual ~IDrawSurface();

        virtual void set_color(uint32_t argb) = 0;
        virtual void draw_line(float x1, float y1, float x2, float y2, float width) = 0;
        virtual void draw_rect(float x, float y, float w, float h, float width) = 0;
        virtual void fill_rect(float x, float y, float w, float h) = 0;
        virtual void draw_round_rect(float x, float y, float w, float h, float rx, float ry, float width) = 0;
        virtual void fill_round_rect(float x, float y, float w, float h, float rx, float ry) = 0;
        virtual void draw_glyph(const GlyphBitmap& glyph, float pen_x, float pen_y, float angle) = 0;
    };
} // namespace TexGui
