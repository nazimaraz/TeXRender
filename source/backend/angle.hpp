#pragma once

namespace TeXRender::detail
{
    inline constexpr auto pi = 3.141592653589793238462643383279502884L;

    [[nodiscard]] constexpr float radians_to_degrees(const float radians) noexcept
    {
        return radians * static_cast<float>(180.L / pi);
    }

    [[nodiscard]] constexpr double radians_to_degrees(const double radians) noexcept
    {
        return radians * static_cast<double>(180.L / pi);
    }
} // namespace TeXRender::detail
