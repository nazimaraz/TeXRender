#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include "texrender/render_handle.hpp"

namespace TeXRender
{
    struct FormulaBox
    {
        int width = 0;
        int height = 0;
        int depth = 0;
        float baseline = 0.f;
    };

    enum class FormulaAnchor : std::uint8_t
    {
        top,
        baseline,
        center,
        bottom
    };

    namespace detail
    {
        [[nodiscard]] inline int saturated_round_to_int(const double value) noexcept
        {
            if (!std::isfinite(value))
                return 0;

            const auto clamped = std::clamp(value, static_cast<double>(std::numeric_limits<int>::min()),
                static_cast<double>(std::numeric_limits<int>::max()));
            return static_cast<int>(std::round(clamped));
        }

        [[nodiscard]] inline int saturated_subtract(const int lhs, const int rhs) noexcept
        {
            return saturated_round_to_int(static_cast<double>(lhs) - static_cast<double>(rhs));
        }
    } // namespace detail

    [[nodiscard]] inline FormulaBox formula_box(const RenderHandle& formula)
    {
        return FormulaBox{formula.width(), formula.height(), formula.depth(), formula.baseline()};
    }

    [[nodiscard]] inline int top_for_baseline(const RenderHandle& formula, const int baseline_y)
    {
        return detail::saturated_round_to_int(static_cast<double>(baseline_y) - static_cast<double>(formula.baseline()));
    }

    [[nodiscard]] inline int aligned_top(const RenderHandle& formula, const int y, const FormulaAnchor anchor)
    {
        switch (anchor)
        {
            case FormulaAnchor::top:
                return y;
            case FormulaAnchor::baseline:
                return top_for_baseline(formula, y);
            case FormulaAnchor::center:
                return detail::saturated_subtract(y, formula.height() / 2);
            case FormulaAnchor::bottom:
                return detail::saturated_subtract(y, formula.height());
        }

        return y;
    }

    inline void draw_at_baseline(const RenderHandle& formula, const int x, const int baseline_y)
    {
        formula.draw(x, top_for_baseline(formula, baseline_y));
    }

    inline void draw_aligned(const RenderHandle& formula, const int x, const int y, const FormulaAnchor anchor)
    {
        formula.draw(x, aligned_top(formula, y, anchor));
    }
} // namespace TeXRender
