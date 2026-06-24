#pragma once

#include <cstddef>
#include <list>
#include <unordered_map>
#include "texrender/draw_surface.hpp"
#include "texrender/export.hpp"

struct SDL_Renderer;
struct SDL_Texture;

namespace TeXRender
{
    TEXRENDER_EXPORT void set_default_sdl_renderer(SDL_Renderer* renderer) noexcept;
    [[nodiscard]] TEXRENDER_EXPORT SDL_Renderer* default_sdl_renderer() noexcept;

    class TEXRENDER_EXPORT Sdl2Surface final : public IDrawSurface
    {
    public:
        Sdl2Surface();
        explicit Sdl2Surface(SDL_Renderer* renderer);
        Sdl2Surface(const Sdl2Surface&) = delete;
        Sdl2Surface(Sdl2Surface&&) = delete;
        Sdl2Surface& operator=(const Sdl2Surface&) = delete;
        Sdl2Surface& operator=(Sdl2Surface&&) = delete;
        ~Sdl2Surface() noexcept override;

        void set_color(uint32_t argb) override;
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
        [[nodiscard]] SDL_Texture* texture_for(const GlyphBitmap& glyph);
        void evict_excess_textures();

        struct CachedTexture
        {
            SDL_Texture* texture = nullptr;
            std::list<uint64_t>::iterator lru_it{};
        };

        SDL_Renderer* renderer_ = nullptr;
        std::unordered_map<uint64_t, CachedTexture> textures_;
        std::list<uint64_t> lru_;
        std::size_t max_textures_ = 1024;
        uint8_t r_ = 0;
        uint8_t g_ = 0;
        uint8_t b_ = 0;
        uint8_t a_ = 255;
    };
} // namespace TeXRender
