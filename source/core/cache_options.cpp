#include "texgui/cache_options.hpp"
#include <algorithm>
#include <mutex>

using namespace TexGui;

namespace
{
    std::mutex g_cache_limits_mutex;
    CacheLimits g_cache_limits;

    [[nodiscard]] CacheLimits normalized(CacheLimits limits)
    {
        limits.max_cpu_glyphs = std::max<std::size_t>(1, limits.max_cpu_glyphs);
        limits.max_gpu_textures = std::max<std::size_t>(1, limits.max_gpu_textures);
        return limits;
    }
} // namespace

CacheLimits TexGui::cache_limits() noexcept
{
    const auto lock = std::scoped_lock{g_cache_limits_mutex};
    return g_cache_limits;
}

void TexGui::set_cache_limits(CacheLimits limits) noexcept
{
    const auto lock = std::scoped_lock{g_cache_limits_mutex};
    g_cache_limits = normalized(limits);
}
