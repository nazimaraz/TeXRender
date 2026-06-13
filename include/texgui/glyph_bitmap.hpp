#pragma once

#include <cstdint>

namespace TexGui
{
    /// Borrowed grayscale glyph bitmap produced by texgui's FreeType cache.
    ///
    /// The buffer is owned by the cache and remains valid for the duration of
    /// the draw call. `left` and `top` are FreeType bitmap bearings relative to
    /// the glyph pen. `pitch` is the number of bytes between bitmap rows.
    struct GlyphBitmap
    {
        /// Pointer to 8-bit coverage values, one byte per pixel.
        const unsigned char* buffer{};
        /// Bitmap width in pixels.
        int width = 0;
        /// Bitmap height in pixels.
        int height = 0;
        /// Bytes per bitmap row.
        int pitch = 0;
        /// Horizontal bearing from pen position to bitmap origin.
        int left = 0;
        /// Vertical bearing from baseline to bitmap top.
        int top = 0;
        /// Stable cache key for backend-side glyph caches.
        uint64_t key = 0;
    };
} // namespace TexGui
