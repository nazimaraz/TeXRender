#pragma once

#include <cstdint>

namespace TeXRender
{
    struct GlyphBitmap
    {
        const unsigned char* buffer{};
        int width = 0;
        int height = 0;
        int pitch = 0;
        int left = 0;
        int top = 0;
        std::uint64_t key = 0;
    };
} // namespace TeXRender
