#pragma once

#include <cstddef>
#include "texgui/export.hpp"

namespace TexGui
{
    /// Process-wide cache limits used by the shared FreeType glyph cache and
    /// newly-created backend surfaces.
    struct CacheLimits
    {
        static constexpr std::size_t default_cpu_glyphs = 1024;
        static constexpr std::size_t default_gpu_textures = 1024;

        /// Maximum rasterized FreeType glyph bitmaps retained process-wide.
        std::size_t max_cpu_glyphs = default_cpu_glyphs;
        /// Default maximum GPU/backend glyph textures for newly-created surfaces.
        std::size_t max_gpu_textures = default_gpu_textures;
    };

    /// Return the current process-wide cache limits.
    [[nodiscard]] TEXGUI_EXPORT CacheLimits cache_limits() noexcept;
    /// Set process-wide cache limits. Zero values are clamped to one.
    TEXGUI_EXPORT void set_cache_limits(CacheLimits limits) noexcept;
} // namespace TexGui
