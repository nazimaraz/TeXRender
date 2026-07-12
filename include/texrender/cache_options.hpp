#pragma once

#include <cstddef>
#include "texrender/export.hpp"

namespace TeXRender
{
    struct CacheLimits
    {
        static constexpr auto default_cpu_glyphs = std::size_t{1'024};
        static constexpr auto default_gpu_textures = std::size_t{1'024};
        std::size_t max_cpu_glyphs{default_cpu_glyphs};
        std::size_t max_gpu_textures{default_gpu_textures};
    };

    [[nodiscard]] TEXRENDER_EXPORT CacheLimits cache_limits() noexcept;
    TEXRENDER_EXPORT void set_cache_limits(CacheLimits limits) noexcept;
} // namespace TeXRender
