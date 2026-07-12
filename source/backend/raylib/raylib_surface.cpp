#include "texrender/raylib_surface.hpp"
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <rlgl.h>
#include <vector>
#include "backend/angle.hpp"
#include "backend/glyph_bitmap_validation.hpp"
#include "render/glyph_cache.hpp"
#include "texrender/cache_options.hpp"

using namespace TeXRender;

namespace
{
    [[nodiscard]] Color to_raylib(const std::uint32_t argb)
    {
        return {
            static_cast<unsigned char>((argb >> 16) & 0xff),
            static_cast<unsigned char>((argb >> 8) & 0xff),
            static_cast<unsigned char>(argb & 0xff),
            static_cast<unsigned char>((argb >> 24) & 0xff),
        };
    }

    [[nodiscard]] bool finite_line(const float x1, const float y1, const float x2, const float y2, const float width)
    {
        return std::isfinite(x1) && std::isfinite(y1) && std::isfinite(x2) && std::isfinite(y2) && std::isfinite(width);
    }

    [[nodiscard]] bool finite_rect(const float x, const float y, const float w, const float h)
    {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(w) && std::isfinite(h) && w > 0.f && h > 0.f;
    }
} // namespace

RaylibSurface::RaylibSurface()
    : max_textures_{std::max<std::size_t>(1, cache_limits().max_gpu_textures)}
{}

RaylibSurface::~RaylibSurface() noexcept
{
    unload();
}

void RaylibSurface::unload() noexcept
{
    if (IsWindowReady())
    {
        for (const auto& entry : textures_)
            UnloadTexture(entry.second.texture);
    }

    textures_.clear();
    lru_.clear();
}

void RaylibSurface::set_max_textures(const std::size_t max_textures)
{
    max_textures_ = std::max<std::size_t>(1, max_textures);
    evict_excess_textures();
}

std::size_t RaylibSurface::max_textures() const noexcept
{
    return max_textures_;
}

void RaylibSurface::evict_excess_textures()
{
    if (textures_.size() <= max_textures_)
        return;

    if (IsWindowReady())
        rlDrawRenderBatchActive();

    while (textures_.size() > max_textures_)
    {
        if (const auto victim = textures_.find(lru_.back()); victim != textures_.end())
        {
            if (IsWindowReady())
                UnloadTexture(victim->second.texture);
            textures_.erase(victim);
        }
        lru_.pop_back();
    }
}

void RaylibSurface::set_color(const std::uint32_t argb)
{
    color_ = to_raylib(argb);
}

void RaylibSurface::draw_line(const float x1, const float y1, const float x2, const float y2, const float width)
{
    if (!finite_line(x1, y1, x2, y2, width))
        return;

    DrawLineEx(Vector2{x1, y1}, Vector2{x2, y2}, width, color_);
}

void RaylibSurface::draw_rect(const float x, const float y, const float w, const float h, const float width)
{
    if (!finite_rect(x, y, w, h) || !std::isfinite(width))
        return;

    DrawRectangleLinesEx(Rectangle{x, y, w, h}, width, color_);
}

void RaylibSurface::fill_rect(const float x, const float y, const float w, const float h)
{
    if (!finite_rect(x, y, w, h))
        return;

    DrawRectangleRec(Rectangle{x, y, w, h}, color_);
}

void RaylibSurface::draw_round_rect(const float x, const float y, const float w, const float h, const float rx, const float ry,
    const float width)
{
    if (!finite_rect(x, y, w, h) || !std::isfinite(rx) || !std::isfinite(ry) || !std::isfinite(width))
        return;

    const auto shorter = std::min(w, h);
    const auto roundness = shorter > 0.f ? std::min(1.f, (std::min(rx, ry) * 2.f) / shorter) : 0.f;
    DrawRectangleRoundedLinesEx(Rectangle{x, y, w, h}, roundness, 8, width, color_);
}

void RaylibSurface::fill_round_rect(const float x, const float y, const float w, const float h, const float rx, const float ry)
{
    if (!finite_rect(x, y, w, h) || !std::isfinite(rx) || !std::isfinite(ry))
        return;

    const auto shorter = std::min(w, h);
    const auto roundness = shorter > 0.f ? std::min(1.f, (std::min(rx, ry) * 2.f) / shorter) : 0.f;
    DrawRectangleRounded(Rectangle{x, y, w, h}, roundness, 8, color_);
}

const Texture2D* RaylibSurface::texture_for(const GlyphBitmap& glyph)
{
    if (const auto found = textures_.find(glyph.key); found != textures_.end())
    {
        lru_.splice(lru_.begin(), lru_, found->second.lru_it); // mark most-recently-used
        return &found->second.texture;
    }

    upload_pixels_.resize(static_cast<std::size_t>(glyph.width) * static_cast<std::size_t>(glyph.height) * 4u);
    for (auto row = 0; row < glyph.height; ++row)
    {
        for (auto col = 0; col < glyph.width; ++col)
        {
            const auto coverage =
                glyph.buffer[static_cast<std::size_t>(row) * static_cast<std::size_t>(glyph.pitch) + static_cast<std::size_t>(col)];
            const auto dst =
                (static_cast<std::size_t>(row) * static_cast<std::size_t>(glyph.width) + static_cast<std::size_t>(col)) * 4u;
            upload_pixels_[dst + 0] = 255;
            upload_pixels_[dst + 1] = 255;
            upload_pixels_[dst + 2] = 255;
            upload_pixels_[dst + 3] = coverage;
        }
    }

    auto image = Image{};
    image.data = upload_pixels_.data();
    image.width = glyph.width;
    image.height = glyph.height;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    const auto texture = LoadTextureFromImage(image);
    if (texture.id == 0)
    {
        detail::debug_log("RaylibSurface: failed to upload glyph texture");
        return nullptr;
    }

    lru_.push_front(glyph.key);
    const auto it = textures_.emplace(glyph.key, CachedTexture{texture, lru_.begin()}).first;
    evict_excess_textures();
    return &it->second.texture;
}

void RaylibSurface::draw_glyph(const GlyphBitmap& glyph, const float pen_x, const float pen_y, const float angle)
{
    if (!IsWindowReady() || !detail::valid_cacheable_glyph_bitmap_for_rgba(glyph) || !std::isfinite(pen_x) ||
        !std::isfinite(pen_y) || !std::isfinite(angle))
        return;

    const auto* const texture = texture_for(glyph);
    if (!texture)
        return;

    const auto source = Rectangle{0.f, 0.f, static_cast<float>(glyph.width), static_cast<float>(glyph.height)};
    const auto dest = Rectangle{pen_x, pen_y, static_cast<float>(glyph.width), static_cast<float>(glyph.height)};
    const auto origin = Vector2{static_cast<float>(-glyph.left), static_cast<float>(glyph.top)};
    DrawTexturePro(*texture, source, dest, origin, detail::radians_to_degrees(angle), color_);
}
