#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "texgui/draw_surface.hpp"
#include "texgui/export.hpp"

namespace TexGui
{
    /// IDrawSurface implementation that records draw calls into an SVG document.
    ///
    /// Lines and rectangles are emitted as native SVG elements. Glyphs arrive at
    /// this backend as FreeType bitmaps, so they are embedded as small RGBA PNG
    /// data URIs inside the SVG.
    class TEXGUI_EXPORT SvgSurface final : public IDrawSurface
    {
    public:
        SvgSurface() = default;
        SvgSurface(int width, int height);

        void set_color(uint32_t argb) override;
        void draw_line(float x1, float y1, float x2, float y2, float width) override;
        void draw_rect(float x, float y, float w, float h, float width) override;
        void fill_rect(float x, float y, float w, float h) override;
        void draw_round_rect(float x, float y, float w, float h, float rx, float ry, float width) override;
        void fill_round_rect(float x, float y, float w, float h, float rx, float ry) override;
        void draw_glyph(const GlyphBitmap& glyph, float pen_x, float pen_y, float angle) override;

        /// Set the SVG viewport dimensions in pixels.
        void resize(int width, int height);
        /// Clear recorded elements and optionally set a background color.
        void clear(uint32_t argb = 0x00000000);

        /// SVG viewport width in pixels.
        [[nodiscard]] int width() const;
        /// SVG viewport height in pixels.
        [[nodiscard]] int height() const;
        /// Number of recorded draw elements, excluding the root svg element.
        [[nodiscard]] std::size_t element_count() const;
        /// Return the complete SVG document.
        [[nodiscard]] std::string svg() const;
        /// Write the complete SVG document to disk.
        [[nodiscard]] bool write_svg(const std::string& path) const;

    private:
        [[nodiscard]] std::string paint_attributes(const char* paint_kind) const;

        int width_ = 0;
        int height_ = 0;
        uint32_t color_ = 0xff000000;
        std::optional<uint32_t> background_;
        std::vector<std::string> elements_;
    };
} // namespace TexGui
