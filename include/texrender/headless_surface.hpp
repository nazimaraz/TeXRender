#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include "texrender/draw_surface.hpp"
#include "texrender/export.hpp"

namespace TeXRender
{
    class TEXRENDER_EXPORT HeadlessSurface final : public IDrawSurface
    {
    public:
        HeadlessSurface() = default;

        void set_color(std::uint32_t argb) override;
        void draw_line(float x1, float y1, float x2, float y2, float width) override;
        void draw_rect(float x, float y, float w, float h, float width) override;
        void fill_rect(float x, float y, float w, float h) override;
        void draw_round_rect(float x, float y, float w, float h, float rx, float ry, float width) override;
        void fill_round_rect(float x, float y, float w, float h, float rx, float ry) override;
        void draw_glyph(const GlyphBitmap& glyph, float pen_x, float pen_y, float angle) override;
        void resize(int width, int height);
        void clear(std::uint32_t argb = 0x00000000);
        [[nodiscard]] int width() const noexcept;
        [[nodiscard]] int height() const noexcept;
        [[nodiscard]] const std::vector<unsigned char>& pixels() const noexcept;
        [[nodiscard]] std::size_t covered_pixels() const noexcept;
        [[nodiscard]] bool write_ppm(const std::string& path) const;
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
