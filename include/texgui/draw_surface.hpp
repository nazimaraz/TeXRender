#pragma once

#include <cstdint>
#include "texgui/export.hpp"
#include "texgui/glyph_bitmap.hpp"

namespace TexGui
{
    /// Backend interface used by MicroTeX draw commands after texgui has
    /// applied transforms, color state, and FreeType glyph rasterization.
    ///
    /// Coordinates are screen pixels in the target surface. Implementations can
    /// be GPU backed, like RaylibSurface, or purely software, like
    /// HeadlessSurface. Methods are called synchronously from
    /// RenderHandle::draw(), so custom backends do not need to store draw
    /// commands unless their own rendering API requires it.
    class TEXGUI_EXPORT IDrawSurface
    {
    public:
        IDrawSurface();
        IDrawSurface(const IDrawSurface&) = delete;
        IDrawSurface(IDrawSurface&&) = delete;
        IDrawSurface& operator=(const IDrawSurface&) = delete;
        IDrawSurface& operator=(IDrawSurface&&) = delete;
        virtual ~IDrawSurface();

        /// Set the current drawing color as 0xAARRGGBB.
        virtual void set_color(uint32_t argb) = 0;
        /// Draw a line segment using the current color and pixel width.
        virtual void draw_line(float x1, float y1, float x2, float y2, float width) = 0;
        /// Stroke an axis-aligned rectangle using the current color.
        virtual void draw_rect(float x, float y, float w, float h, float width) = 0;
        /// Fill an axis-aligned rectangle using the current color.
        virtual void fill_rect(float x, float y, float w, float h) = 0;
        /// Stroke a rounded rectangle using the current color.
        virtual void draw_round_rect(float x, float y, float w, float h, float rx, float ry, float width) = 0;
        /// Fill a rounded rectangle using the current color.
        virtual void fill_round_rect(float x, float y, float w, float h, float rx, float ry) = 0;
        /// Draw a pre-rasterized glyph with its pen position and rotation angle.
        virtual void draw_glyph(const GlyphBitmap& glyph, float pen_x, float pen_y, float angle) = 0;
    };
} // namespace TexGui
