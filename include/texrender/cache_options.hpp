#pragma once

#include <cstddef>
#include "texrender/export.hpp"

namespace TeXRender
{
    /// Process-wide cache limits used by the shared FreeType glyph cache and
    /// newly-created backend surfaces.
    ///
    /// These limits are intentionally simple. Tune them for applications that
    /// render many unique formulas or run in a memory-constrained environment.
    struct CacheLimits
    {
        /// Default maximum rasterized FreeType glyphs retained process-wide.
        static constexpr std::size_t default_cpu_glyphs = 1024;
        /// Default maximum backend/GPU glyph textures retained per surface.
        static constexpr std::size_t default_gpu_textures = 1024;

        /// Maximum rasterized FreeType glyph bitmaps retained process-wide.
        std::size_t max_cpu_glyphs = default_cpu_glyphs;
        /// Default maximum GPU/backend glyph textures for newly-created surfaces.
        std::size_t max_gpu_textures = default_gpu_textures;
    };

    /// Return the current process-wide cache limits.
    [[nodiscard]] TEXRENDER_EXPORT CacheLimits cache_limits() noexcept;
    /// Set process-wide cache limits. Zero values are clamped to one.
    ///
    /// Newly-created GUI surfaces pick up `max_gpu_textures`; existing surfaces
    /// can be tuned directly with their `set_max_textures()` member.
    TEXRENDER_EXPORT void set_cache_limits(CacheLimits limits) noexcept;
} // namespace TeXRender
