#pragma once

#include <cstddef>
#include <cstdint>
#include <list>
#include <unordered_map>
#include "texgui/draw_surface.hpp"
#include "texgui/export.hpp"

struct SDL_Renderer;
struct SDL_Texture;

namespace TexGui
{
    /// Set the renderer used when LatexDocument creates the default SDL2 surface.
    ///
    /// Applications can either call this before constructing a default
    /// `LatexDocument`, or inject `std::make_unique<Sdl2Surface>(renderer)`.
    TEXGUI_EXPORT void set_default_sdl_renderer(SDL_Renderer* renderer) noexcept;

    /// Return the renderer currently used by default SDL2 surfaces.
    [[nodiscard]] TEXGUI_EXPORT SDL_Renderer* default_sdl_renderer() noexcept;

    /// SDL2-backed drawing surface for rendering formulas into an SDL_Renderer.
    ///
    /// The renderer is borrowed and must outlive the surface and every
    /// RenderHandle using it. Destroy texgui handles/documents before destroying
    /// the renderer so cached glyph textures can be released safely.
    class TEXGUI_EXPORT Sdl2Surface final : public IDrawSurface
    {
    public:
        /// Construct from the process default renderer set by set_default_sdl_renderer().
        Sdl2Surface();
        /// Construct from an explicit borrowed SDL_Renderer.
        explicit Sdl2Surface(SDL_Renderer* renderer);
        Sdl2Surface(const Sdl2Surface&) = delete;
        Sdl2Surface(Sdl2Surface&&) = delete;
        Sdl2Surface& operator=(const Sdl2Surface&) = delete;
        Sdl2Surface& operator=(Sdl2Surface&&) = delete;
        ~Sdl2Surface() override;

        void set_color(uint32_t argb) override;
        void draw_line(float x1, float y1, float x2, float y2, float width) override;
        void draw_rect(float x, float y, float w, float h, float width) override;
        void fill_rect(float x, float y, float w, float h) override;
        void draw_round_rect(float x, float y, float w, float h, float rx, float ry, float width) override;
        void fill_round_rect(float x, float y, float w, float h, float rx, float ry) override;
        void draw_glyph(const GlyphBitmap& glyph, float pen_x, float pen_y, float angle) override;

        /// Release all cached SDL textures. Safe to call more than once.
        void unload();
        /// Set the maximum number of cached glyph textures for this surface.
        void set_max_textures(std::size_t max_textures);
        /// Maximum cached glyph textures for this surface.
        [[nodiscard]] std::size_t max_textures() const;

    private:
        [[nodiscard]] SDL_Texture* texture_for(const GlyphBitmap& glyph);
        void evict_excess_textures();

        struct CachedTexture
        {
            SDL_Texture* texture = nullptr;
            std::list<uint64_t>::iterator lru_it;
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
} // namespace TexGui
