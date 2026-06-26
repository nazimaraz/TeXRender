#pragma once

#include <list>
#include <cstddef>
#include <raylib.h>
#include <unordered_map>
#include "texrender/draw_surface.hpp"
#include "texrender/export.hpp"

namespace TeXRender
{
    class TEXRENDER_EXPORT RaylibSurface final : public IDrawSurface
    {
    public:
        RaylibSurface();
        RaylibSurface(const RaylibSurface&) = delete;
        RaylibSurface(RaylibSurface&&) = delete;
        RaylibSurface& operator=(const RaylibSurface&) = delete;
        RaylibSurface& operator=(RaylibSurface&&) = delete;
        ~RaylibSurface() noexcept override;

        void set_color(std::uint32_t argb) override;
        void draw_line(float x1, float y1, float x2, float y2, float width) override;
        void draw_rect(float x, float y, float w, float h, float width) override;
        void fill_rect(float x, float y, float w, float h) override;
        void draw_round_rect(float x, float y, float w, float h, float rx, float ry, float width) override;
        void fill_round_rect(float x, float y, float w, float h, float rx, float ry) override;
        void draw_glyph(const GlyphBitmap& glyph, float pen_x, float pen_y, float angle) override;
        void unload() noexcept;
        void set_max_textures(std::size_t max_textures);
        [[nodiscard]] std::size_t max_textures() const noexcept;

    private:
        [[nodiscard]] const Texture2D* texture_for(const GlyphBitmap& glyph);
        void evict_excess_textures();

        struct CachedTexture
        {
            Texture2D texture{};
            std::list<std::uint64_t>::iterator lru_it;
        };

        std::unordered_map<std::uint64_t, CachedTexture> textures_;
        std::list<std::uint64_t> lru_;
        std::size_t max_textures_ = 1024;
        Color color_{0, 0, 0, 255};
    };
} // namespace TeXRender
