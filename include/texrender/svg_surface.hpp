#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "texrender/draw_surface.hpp"
#include "texrender/export.hpp"

namespace TeXRender
{
    class TEXRENDER_EXPORT SvgSurface final : public IDrawSurface
    {
    public:
        SvgSurface() = default;
        SvgSurface(int width, int height);

        void set_color(std::uint32_t argb) override;
        void draw_line(float x1, float y1, float x2, float y2, float stroke_width) override;
        void draw_rect(float x, float y, float w, float h, float stroke_width) override;
        void fill_rect(float x, float y, float w, float h) override;
        void draw_round_rect(float x, float y, float w, float h, float rx, float ry, float stroke_width) override;
        void fill_round_rect(float x, float y, float w, float h, float rx, float ry) override;
        void draw_glyph(const GlyphBitmap& glyph, float pen_x, float pen_y, float angle) override;
        void resize(int width, int height);
        void clear(std::uint32_t argb = 0x00000000);
        [[nodiscard]] int width() const noexcept;
        [[nodiscard]] int height() const noexcept;
        [[nodiscard]] std::size_t element_count() const noexcept;
        [[nodiscard]] std::string svg() const;
        [[nodiscard]] bool write_svg(const std::string& path) const;

    private:
        struct GlyphDefKey
        {
            std::uint64_t glyph_key{};
            std::uint32_t argb{};

            [[nodiscard]] bool operator==(const GlyphDefKey& other) const noexcept
            {
                return glyph_key == other.glyph_key && argb == other.argb;
            }
        };

        struct GlyphDefKeyHash
        {
            [[nodiscard]] std::size_t operator()(const GlyphDefKey& key) const noexcept;
        };

        struct GlyphDef
        {
            std::string id;
            std::string element;
        };

        [[nodiscard]] std::optional<std::string> glyph_def_id(const GlyphBitmap& glyph);

        int width_{0};
        int height_{0};
        std::uint32_t color_{0xff000000};
        std::optional<std::uint32_t> background_;
        std::unordered_map<GlyphDefKey, std::size_t, GlyphDefKeyHash> glyph_def_lookup_;
        std::vector<GlyphDef> glyph_defs_;
        std::vector<std::string> elements_;
    };
} // namespace TeXRender
