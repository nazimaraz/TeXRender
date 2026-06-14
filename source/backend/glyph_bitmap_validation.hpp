#pragma once

#include <cstddef>
#include <limits>
#include "texrender/glyph_bitmap.hpp"

namespace TeXRender::detail
{
    [[nodiscard]] inline bool valid_glyph_bitmap(const GlyphBitmap& glyph) noexcept
    {
        if (!glyph.buffer || glyph.width <= 0 || glyph.height <= 0 || glyph.pitch <= 0 || glyph.width > glyph.pitch)
            return false;

        const auto width = static_cast<std::size_t>(glyph.width);
        const auto height = static_cast<std::size_t>(glyph.height);
        const auto pitch = static_cast<std::size_t>(glyph.pitch);
        const auto max = std::numeric_limits<std::size_t>::max();
        return height - 1u <= (max - width) / pitch;
    }

    [[nodiscard]] inline bool valid_glyph_bitmap_for_rgba(const GlyphBitmap& glyph) noexcept
    {
        if (!valid_glyph_bitmap(glyph))
            return false;

        const auto width = static_cast<std::size_t>(glyph.width);
        const auto height = static_cast<std::size_t>(glyph.height);
        const auto max_pixels = std::numeric_limits<std::size_t>::max() / 4u;
        return width <= max_pixels / height;
    }

    [[nodiscard]] inline bool valid_cacheable_glyph_bitmap_for_rgba(const GlyphBitmap& glyph) noexcept
    {
        return glyph.key != 0 && valid_glyph_bitmap_for_rgba(glyph);
    }
} // namespace TeXRender::detail
