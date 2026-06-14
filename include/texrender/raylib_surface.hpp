#pragma once

#include <list>
#include <cstddef>
#include <raylib.h>
#include <unordered_map>
#include "texrender/draw_surface.hpp"
#include "texrender/export.hpp"

namespace TeXRender
{
    /// raylib-backed drawing surface for rendering formulas inside a raylib frame.
    ///
    /// Use this directly when injecting a surface, or let LatexDocument create
    /// one by default when texrender is built with `TEXRENDER_BACKEND=raylib`.
    /// Destroy or unload it before `CloseWindow()` so GPU glyph textures are
    /// released while the raylib context is still alive. Drawing must happen
    /// inside the caller's active raylib frame.
    class TEXRENDER_EXPORT RaylibSurface final : public IDrawSurface
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
        ///
        /// Call this before `CloseWindow()` if the surface may outlive formula
        /// handles or if you want to drop texture memory proactively.
        void unload();
        /// Set the maximum number of cached glyph textures for this surface.
        ///
        /// Zero is clamped to one; lower values trade memory for more texture
        /// uploads during repeated draws.
        void set_max_textures(std::size_t max_textures);
        /// Maximum cached glyph textures for this surface.
        [[nodiscard]] std::size_t max_textures() const;

    private:
        [[nodiscard]] const Texture2D& texture_for(const GlyphBitmap& glyph);
        void evict_excess_textures();

        struct CachedTexture
        {
            Texture2D texture{};
            std::list<uint64_t>::iterator lru_it{};
        };

        std::unordered_map<uint64_t, CachedTexture> textures_;
        std::list<uint64_t> lru_;
        std::size_t max_textures_ = 1024;
        Color color_{0, 0, 0, 255};
    };
} // namespace TeXRender
