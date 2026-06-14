#include "texrender/raylib_surface.hpp"
#include <algorithm>
#include <cstddef>
#include <rlgl.h>
#include <vector>
#include "texrender/cache_options.hpp"

using namespace TeXRender;

namespace
{
    [[nodiscard]] constexpr float to_degrees(const float radians)
    {
        return radians * (180.f / 3.14159265358979323846f);
    }

    [[nodiscard]] Color to_raylib(const uint32_t argb)
    {
        return {
            static_cast<unsigned char>((argb >> 16) & 0xff),
            static_cast<unsigned char>((argb >> 8) & 0xff),
            static_cast<unsigned char>(argb & 0xff),
            static_cast<unsigned char>((argb >> 24) & 0xff),
        };
    }
} // namespace

RaylibSurface::RaylibSurface()
    : max_textures_{std::max<size_t>(1, cache_limits().max_gpu_textures)}
{}

RaylibSurface::~RaylibSurface()
{
    unload();
}

void RaylibSurface::unload()
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

std::size_t RaylibSurface::max_textures() const
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

void RaylibSurface::set_color(const uint32_t argb)
{
    color_ = to_raylib(argb);
}

void RaylibSurface::draw_line(const float x1, const float y1, const float x2, const float y2, const float width)
{
    DrawLineEx(Vector2{x1, y1}, Vector2{x2, y2}, width, color_);
}

void RaylibSurface::draw_rect(const float x, const float y, const float w, const float h, const float width)
{
    DrawRectangleLinesEx(Rectangle{x, y, w, h}, width, color_);
}

void RaylibSurface::fill_rect(const float x, const float y, const float w, const float h)
{
    DrawRectangleRec(Rectangle{x, y, w, h}, color_);
}

void RaylibSurface::draw_round_rect(const float x, const float y, const float w, const float h, const float rx, const float ry,
    const float width)
{
    const auto shorter = std::min(w, h);
    const auto roundness = shorter > 0.f ? std::min(1.f, (std::min(rx, ry) * 2.f) / shorter) : 0.f;
    DrawRectangleRoundedLinesEx(Rectangle{x, y, w, h}, roundness, 8, width, color_);
}

void RaylibSurface::fill_round_rect(const float x, const float y, const float w, const float h, const float rx, const float ry)
{
    const auto shorter = std::min(w, h);
    const auto roundness = shorter > 0.f ? std::min(1.f, (std::min(rx, ry) * 2.f) / shorter) : 0.f;
    DrawRectangleRounded(Rectangle{x, y, w, h}, roundness, 8, color_);
}

const Texture2D& RaylibSurface::texture_for(const GlyphBitmap& glyph)
{
    if (const auto found = textures_.find(glyph.key); found != textures_.end())
    {
        lru_.splice(lru_.begin(), lru_, found->second.lru_it); // mark most-recently-used
        return found->second.texture;
    }

    auto rgba = std::vector<unsigned char>(static_cast<size_t>(glyph.width) * static_cast<size_t>(glyph.height) * 4u);
    for (auto row = 0; row < glyph.height; ++row)
    {
        for (auto col = 0; col < glyph.width; ++col)
        {
            const auto coverage =
                glyph.buffer[static_cast<size_t>(row) * static_cast<size_t>(glyph.pitch) + static_cast<size_t>(col)];
            const auto dst = (static_cast<size_t>(row) * static_cast<size_t>(glyph.width) + static_cast<size_t>(col)) * 4u;
            rgba[dst + 0] = 255;
            rgba[dst + 1] = 255;
            rgba[dst + 2] = 255;
            rgba[dst + 3] = coverage;
        }
    }

    auto image = Image{};
    image.data = rgba.data();
    image.width = glyph.width;
    image.height = glyph.height;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    const auto texture = LoadTextureFromImage(image);

    lru_.push_front(glyph.key);
    const auto emplaced = textures_.emplace(glyph.key, CachedTexture{texture, lru_.begin()});
    const auto it = emplaced.first;
    evict_excess_textures();

    return it->second.texture;
}

void RaylibSurface::draw_glyph(const GlyphBitmap& glyph, const float pen_x, const float pen_y, const float angle)
{
    if (!glyph.buffer || glyph.width <= 0 || glyph.height <= 0)
        return;

    const auto& texture = texture_for(glyph);
    const auto source = Rectangle{0.f, 0.f, static_cast<float>(glyph.width), static_cast<float>(glyph.height)};
    const auto dest = Rectangle{pen_x, pen_y, static_cast<float>(glyph.width), static_cast<float>(glyph.height)};
    const auto origin = Vector2{static_cast<float>(-glyph.left), static_cast<float>(glyph.top)};
    DrawTexturePro(texture, source, dest, origin, to_degrees(angle), color_);
}
