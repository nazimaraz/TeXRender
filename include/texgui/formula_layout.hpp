#pragma once

#include <cmath>
#include "texgui/render_handle.hpp"

namespace TexGui
{
    /// Public formula metrics collected from a RenderHandle.
    struct FormulaBox
    {
        int width = 0;
        int height = 0;
        int depth = 0;
        float baseline = 0.f;
    };

    /// Vertical anchor used by layout helpers.
    enum class FormulaAnchor
    {
        top,
        baseline,
        center,
        bottom
    };

    /// Snapshot a formula's current metrics.
    [[nodiscard]] inline FormulaBox formula_box(const RenderHandle& formula)
    {
        return FormulaBox{formula.width(), formula.height(), formula.depth(), formula.baseline()};
    }

    /// Return the top-left y coordinate needed to place a formula on a baseline.
    [[nodiscard]] inline int top_for_baseline(const RenderHandle& formula, const int baseline_y)
    {
        return static_cast<int>(std::lround(static_cast<float>(baseline_y) - formula.baseline()));
    }

    /// Return the top-left y coordinate for a chosen vertical anchor.
    [[nodiscard]] inline int aligned_top(const RenderHandle& formula, const int y, const FormulaAnchor anchor)
    {
        switch (anchor)
        {
            case FormulaAnchor::top:
                return y;
            case FormulaAnchor::baseline:
                return top_for_baseline(formula, y);
            case FormulaAnchor::center:
                return y - formula.height() / 2;
            case FormulaAnchor::bottom:
                return y - formula.height();
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
} // namespace TexGui
