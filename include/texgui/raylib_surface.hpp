#pragma once

#include <list>
#include <raylib.h>
#include <unordered_map>
#include "texgui/draw_surface.hpp"

namespace TexGui
{
    /// raylib-backed drawing surface for rendering formulas inside a raylib frame.
    ///
    /// Use this directly when injecting a surface, or let LatexDocument create
    /// one by default when texgui is built with `TEXGUI_BACKEND=raylib`.
    /// Destroy or unload it before `CloseWindow()` so GPU glyph textures are
    /// released while the raylib context is still alive.
    class RaylibSurface final : public IDrawSurface
    {
    public:
        RaylibSurface();
        RaylibSurface(const RaylibSurface&) = delete;
        RaylibSurface(RaylibSurface&&) = delete;
        RaylibSurface& operator=(const RaylibSurface&) = delete;
        RaylibSurface& operator=(RaylibSurface&&) = delete;
        ~RaylibSurface() override;

        void set_color(uint32_t argb) override;
        void draw_line(float x1, float y1, float x2, float y2, float width) override;
        void draw_rect(float x, float y, float w, float h, float width) override;
        void fill_rect(float x, float y, float w, float h) override;
        void draw_round_rect(float x, float y, float w, float h, float rx, float ry, float width) override;
        void fill_round_rect(float x, float y, float w, float h, float rx, float ry) override;
        void draw_glyph(const GlyphBitmap& glyph, float pen_x, float pen_y, float angle) override;
        /// Release all cached GPU textures. Safe to call more than once.
        void unload();

    private:
        [[nodiscard]] const Texture2D& texture_for(const GlyphBitmap& glyph);

        struct CachedTexture
        {
            Texture2D texture{};
            std::list<uint64_t>::iterator lru_it;
        };

        static constexpr size_t k_max_textures = 1024;
        static_assert(k_max_textures >= 1);
        std::unordered_map<uint64_t, CachedTexture> textures_;
        std::list<uint64_t> lru_;
        Color color_{0, 0, 0, 255};
    };
} // namespace TexGui
