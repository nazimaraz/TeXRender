#include "texrender/sdl2_surface.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <SDL.h>
#include "backend/glyph_bitmap_validation.hpp"
#include "render/glyph_cache.hpp"
#include "texrender/cache_options.hpp"

using namespace TeXRender;

namespace
{
    std::atomic<SDL_Renderer*> g_default_renderer{nullptr};

    [[nodiscard]] constexpr double to_degrees(const float radians)
    {
        return static_cast<double>(radians) * (180.0 / 3.14159265358979323846);
    }

    [[nodiscard]] unsigned char channel(const uint32_t argb, const unsigned shift)
    {
        return static_cast<unsigned char>((argb >> shift) & 0xffu);
    }

    void ensure_renderer(const SDL_Renderer* const renderer)
    {
        if (!renderer)
            throw std::invalid_argument("texrender: Sdl2Surface requires an SDL_Renderer");
    }

    [[nodiscard]] SDL_FRect rect(const float x, const float y, const float w, const float h)
    {
        return SDL_FRect{x, y, w, h};
    }

    [[nodiscard]] bool finite_line(const float x1, const float y1, const float x2, const float y2, const float width)
    {
        return std::isfinite(x1) && std::isfinite(y1) && std::isfinite(x2) && std::isfinite(y2) && std::isfinite(width);
    }

    [[nodiscard]] bool finite_rect(const float x, const float y, const float w, const float h)
    {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(w) && std::isfinite(h) && w > 0.f && h > 0.f;
    }

    [[nodiscard]] int stroke_steps(const float width)
    {
        if (!std::isfinite(width))
            return 0;

        constexpr auto max_stroke_width = 4096.f;
        return static_cast<int>(std::ceil(std::clamp(width, 1.f, max_stroke_width)));
    }
} // namespace

void TeXRender::set_default_sdl_renderer(SDL_Renderer* const renderer) noexcept
{
    g_default_renderer.store(renderer, std::memory_order_release);
}

SDL_Renderer* TeXRender::default_sdl_renderer() noexcept
{
    return g_default_renderer.load(std::memory_order_acquire);
}

Sdl2Surface::Sdl2Surface()
    : Sdl2Surface{default_sdl_renderer()}
{}

Sdl2Surface::Sdl2Surface(SDL_Renderer* const renderer)
    : renderer_{renderer}
    , max_textures_{std::max<std::size_t>(1, cache_limits().max_gpu_textures)}
{
    ensure_renderer(renderer_);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
}

Sdl2Surface::~Sdl2Surface() noexcept
{
    unload();
}

void Sdl2Surface::unload() noexcept
{
    for (const auto& entry : textures_)
        SDL_DestroyTexture(entry.second.texture);
    textures_.clear();
    lru_.clear();
}

void Sdl2Surface::set_max_textures(const std::size_t texture_limit)
{
    max_textures_ = std::max<std::size_t>(1, texture_limit);
    evict_excess_textures();
}

std::size_t Sdl2Surface::max_textures() const noexcept
{
    return max_textures_;
}

void Sdl2Surface::evict_excess_textures()
{
    while (textures_.size() > max_textures_)
    {
        if (const auto victim = textures_.find(lru_.back()); victim != textures_.end())
        {
            SDL_DestroyTexture(victim->second.texture);
            textures_.erase(victim);
        }
        lru_.pop_back();
    }
}

void Sdl2Surface::set_color(const uint32_t argb)
{
    r_ = channel(argb, 16u);
    g_ = channel(argb, 8u);
    b_ = channel(argb, 0u);
    a_ = channel(argb, 24u);
    SDL_SetRenderDrawColor(renderer_, r_, g_, b_, a_);
}

void Sdl2Surface::draw_line(const float x1, const float y1, const float x2, const float y2, const float width)
{
    if (!finite_line(x1, y1, x2, y2, width))
        return;

    if (width <= 1.f)
    {
        SDL_RenderDrawLineF(renderer_, x1, y1, x2, y2);
        return;
    }

    const auto dx = x2 - x1;
    const auto dy = y2 - y1;
    const auto length = std::hypot(dx, dy);
    if (length <= 0.f)
    {
        const auto dot = rect(x1 - width / 2.f, y1 - width / 2.f, width, width);
        SDL_RenderFillRectF(renderer_, &dot);
        return;
    }

    const auto nx = -dy / length;
    const auto ny = dx / length;
    const auto count = stroke_steps(width);
    if (count <= 0)
        return;

    const auto start = -(count - 1) / 2.f;
    for (auto i = 0; i < count; ++i)
    {
        const auto offset = start + static_cast<float>(i);
        SDL_RenderDrawLineF(renderer_, x1 + nx * offset, y1 + ny * offset, x2 + nx * offset, y2 + ny * offset);
    }
}

void Sdl2Surface::draw_rect(const float x, const float y, const float w, const float h, const float width)
{
    if (!finite_rect(x, y, w, h))
        return;

    const auto count = stroke_steps(width);
    if (count <= 0)
        return;

    for (auto i = 0; i < count; ++i)
    {
        const auto inset = static_cast<float>(i);
        const auto box = rect(x + inset, y + inset, std::max(0.f, w - 2.f * inset), std::max(0.f, h - 2.f * inset));
        SDL_RenderDrawRectF(renderer_, &box);
    }
}

void Sdl2Surface::fill_rect(const float x, const float y, const float w, const float h)
{
    if (!finite_rect(x, y, w, h))
        return;

    const auto box = rect(x, y, w, h);
    SDL_RenderFillRectF(renderer_, &box);
}

void Sdl2Surface::draw_round_rect(const float x, const float y, const float w, const float h, const float rx, const float ry,
    const float width)
{
    (void) rx;
    (void) ry;
    draw_rect(x, y, w, h, width);
}

void Sdl2Surface::fill_round_rect(const float x, const float y, const float w, const float h, const float rx, const float ry)
{
    (void) rx;
    (void) ry;
    fill_rect(x, y, w, h);
}

SDL_Texture* Sdl2Surface::texture_for(const GlyphBitmap& glyph)
{
    if (const auto found = textures_.find(glyph.key); found != textures_.end())
    {
        lru_.splice(lru_.begin(), lru_, found->second.lru_it);
        return found->second.texture;
    }

    if (glyph.width > std::numeric_limits<int>::max() / 4)
    {
        detail::debug_log("Sdl2Surface: glyph bitmap is too wide for SDL2 surface upload");
        return nullptr;
    }

    try
    {
        auto rgba = std::vector<unsigned char>(static_cast<std::size_t>(glyph.width) * static_cast<std::size_t>(glyph.height) * 4u);
        for (auto row = 0; row < glyph.height; ++row)
        {
            for (auto col = 0; col < glyph.width; ++col)
            {
                const auto coverage = glyph.buffer[static_cast<std::size_t>(row) * static_cast<std::size_t>(glyph.pitch) +
                                                   static_cast<std::size_t>(col)];
                const auto dst =
                    (static_cast<std::size_t>(row) * static_cast<std::size_t>(glyph.width) + static_cast<std::size_t>(col)) * 4u;
                rgba[dst + 0] = 255;
                rgba[dst + 1] = 255;
                rgba[dst + 2] = 255;
                rgba[dst + 3] = coverage;
            }
        }

        auto* const surface =
            SDL_CreateRGBSurfaceWithFormatFrom(rgba.data(), glyph.width, glyph.height, 32, glyph.width * 4, SDL_PIXELFORMAT_RGBA32);
        if (!surface)
        {
            detail::debug_log(std::string{"Sdl2Surface: SDL_CreateRGBSurfaceWithFormatFrom failed: "} + SDL_GetError());
            return nullptr;
        }

        auto* const texture = SDL_CreateTextureFromSurface(renderer_, surface);
        SDL_FreeSurface(surface);
        if (!texture)
        {
            detail::debug_log(std::string{"Sdl2Surface: SDL_CreateTextureFromSurface failed: "} + SDL_GetError());
            return nullptr;
        }

        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

        lru_.push_front(glyph.key);
        const auto [it, inserted] = textures_.emplace(glyph.key, CachedTexture{texture, lru_.begin()});
        (void) inserted;
        evict_excess_textures();

        return it->second.texture;
    } catch (const std::exception& error)
    {
        detail::debug_log(std::string{"Sdl2Surface: failed to prepare glyph texture: "} + error.what());
        return nullptr;
    } catch (...)
    {
        detail::debug_log("Sdl2Surface: failed to prepare glyph texture");
        return nullptr;
    }
}

void Sdl2Surface::draw_glyph(const GlyphBitmap& glyph, const float pen_x, const float pen_y, const float angle)
{
    if (!detail::valid_cacheable_glyph_bitmap_for_rgba(glyph) || !std::isfinite(pen_x) || !std::isfinite(pen_y) ||
        !std::isfinite(angle))
        return;

    auto* const texture = texture_for(glyph);
    if (!texture)
        return;

    SDL_SetTextureColorMod(texture, r_, g_, b_);
    SDL_SetTextureAlphaMod(texture, a_);

    const auto dest = rect(pen_x + static_cast<float>(glyph.left), pen_y - static_cast<float>(glyph.top),
        static_cast<float>(glyph.width), static_cast<float>(glyph.height));
    const auto center = SDL_FPoint{static_cast<float>(-glyph.left), static_cast<float>(glyph.top)};
    SDL_RenderCopyExF(renderer_, texture, nullptr, &dest, to_degrees(angle), &center, SDL_FLIP_NONE);
}
