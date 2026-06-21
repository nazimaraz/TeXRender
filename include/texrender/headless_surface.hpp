#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "texrender/draw_surface.hpp"
#include "texrender/export.hpp"

namespace TeXRender
{
    /// Windowless IDrawSurface that composites into an in-memory RGBA8 canvas.
    ///
    /// Rendering is software alpha-compositing in sRGB space. Glyph rotation is
    /// intentionally ignored because this backend is primarily for reference
    /// rendering, tests, CI, and server-side/offscreen output. Draws are clipped
    /// to the canvas bounds. Construct it directly and inject it into
    /// `LatexDocument` when you need deterministic output without a window.
    class TEXRENDER_EXPORT HeadlessSurface final : public IDrawSurface
    {
    public:
        HeadlessSurface() = default;

        void set_color(uint32_t argb) override;
        void draw_line(float x1, float y1, float x2, float y2, float width) override;
        void draw_rect(float x, float y, float w, float h, float width) override;
        void fill_rect(float x, float y, float w, float h) override;
        void draw_round_rect(float x, float y, float w, float h, float rx, float ry, float width) override;
        void fill_round_rect(float x, float y, float w, float h, float rx, float ry) override;
        void draw_glyph(const GlyphBitmap& glyph, float pen_x, float pen_y, float angle) override;

        /// Resize the canvas and clear it to transparent black.
        ///
        /// Existing pixel contents are discarded. Negative dimensions are
        /// treated as an empty canvas by the implementation.
        void resize(int width, int height);
        /// Fill the whole canvas with an ARGB color.
        ///
        /// Use `0xffffffff` for an opaque white background before PNG export.
        void clear(uint32_t argb = 0x00000000);

        /// Canvas width in pixels.
        [[nodiscard]] int width() const noexcept;
        /// Canvas height in pixels.
        [[nodiscard]] int height() const noexcept;
        /// Raw RGBA8 pixels in row-major order.
        ///
        /// Bytes are stored as red, green, blue, alpha for each pixel.
        [[nodiscard]] const std::vector<unsigned char>& pixels() const noexcept;
        /// Count pixels with non-zero alpha.
        [[nodiscard]] size_t covered_pixels() const noexcept;
        /// Write the current canvas as a binary PPM image.
        [[nodiscard]] bool write_ppm(const std::string& path) const;
        /// Write the current canvas as an RGBA PNG image.
        ///
        /// Returns false if the file cannot be opened or the canvas dimensions
        /// are invalid.
        [[nodiscard]] bool write_png(const std::string& path) const;

    private:
        int width_ = 0;
        int height_ = 0;
        std::vector<unsigned char> pixels_;
        unsigned char r_ = 0;
        unsigned char g_ = 0;
        unsigned char b_ = 0;
        unsigned char a_ = 255;
    };
} // namespace TeXRender
