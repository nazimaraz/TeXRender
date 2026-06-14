#pragma once

#include <algorithm>
#include <cmath>

namespace TeXRender::detail
{
    inline constexpr int max_raster_pixel_size = 8192;

    [[nodiscard]] inline int safe_raster_pixel_size(const float text_size, const float scale = 1.f) noexcept
    {
        const auto raw = static_cast<double>(text_size) * static_cast<double>(scale);
        if (!std::isfinite(raw) || raw <= 0.0)
            return 0;

        const auto clamped = std::clamp(raw, 1.0, static_cast<double>(max_raster_pixel_size));
        return static_cast<int>(std::round(clamped));
    }
} // namespace TeXRender::detail
