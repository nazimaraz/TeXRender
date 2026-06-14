#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include "texrender/render_handle.hpp"

namespace TeXRender
{
    /// Public formula metrics collected from a RenderHandle.
    ///
    /// `height` is the full formula box height. `depth` is the portion below
    /// the baseline, and `baseline` is the y offset from the formula top to the
    /// baseline. Use these values to align formulas with surrounding UI text,
    /// labels, table rows, or HUD elements without depending on MicroTeX types.
    struct FormulaBox
    {
        /// Full formula width in pixels.
        int width = 0;
        /// Full formula height in pixels.
        int height = 0;
        /// Distance below the baseline in pixels.
        int depth = 0;
        /// Distance from the top of the formula box to the baseline in pixels.
        float baseline = 0.f;
    };

    /// Vertical anchor used by layout helpers.
    ///
    /// Anchors describe which part of a formula should coincide with the y
    /// coordinate passed to `draw_aligned()` or `aligned_top()`.
    enum class FormulaAnchor
    {
        /// Align the formula's top edge to y.
        top,
        /// Align the formula baseline to y.
        baseline,
        /// Align the formula's vertical center to y.
        center,
        /// Align the formula's bottom edge to y.
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

    /// Snapshot a formula's current metrics.
    ///
    /// This is a convenience for code that wants to store layout values without
    /// keeping a reference to the handle in a layout pass.
    [[nodiscard]] inline FormulaBox formula_box(const RenderHandle& formula)
    {
        return FormulaBox{formula.width(), formula.height(), formula.depth(), formula.baseline()};
    }

    /// Return the top-left y coordinate needed to place a formula on a baseline.
    ///
    /// This is useful when aligning formulas with ordinary text baselines.
    [[nodiscard]] inline int top_for_baseline(const RenderHandle& formula, const int baseline_y)
    {
        return detail::saturated_round_to_int(static_cast<double>(baseline_y) - static_cast<double>(formula.baseline()));
    }

    /// Return the top-left y coordinate for a chosen vertical anchor.
    ///
    /// The returned value can be passed directly to `RenderHandle::draw()`.
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

    /// Draw a formula with `baseline_y` as the formula baseline.
    inline void draw_at_baseline(const RenderHandle& formula, const int x, const int baseline_y)
    {
        formula.draw(x, top_for_baseline(formula, baseline_y));
    }

    /// Draw a formula using a selected vertical anchor.
    inline void draw_aligned(const RenderHandle& formula, const int x, const int y, const FormulaAnchor anchor)
    {
        formula.draw(x, aligned_top(formula, y, anchor));
    }
} // namespace TeXRender
