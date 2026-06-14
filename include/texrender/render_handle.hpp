#pragma once

#include <cstdint>
#include <memory>
#include "texrender/export.hpp"

namespace tex
{
    class TeXRender;
}

namespace TeXRender
{
    class LatexDocument;

    namespace detail
    {
        class DocumentState;
    }

    /// Move-only handle to one parsed and laid-out formula.
    ///
    /// The handle shares ownership of the producing document's runtime state, so
    /// it stays valid to draw and destroy even if the LatexDocument object is
    /// released first. With GPU-backed surfaces, destroy handles before closing
    /// the graphics context so backend caches can release their resources.
    ///
    /// Drawing is immediate: `draw(x, y)` sends primitives to the document's
    /// surface using the current transform/color state owned by the handle.
    class TEXRENDER_EXPORT RenderHandle
    {
    public:
        ~RenderHandle();

        RenderHandle(RenderHandle&&) noexcept;
        RenderHandle& operator=(RenderHandle&&) noexcept;
        RenderHandle(const RenderHandle&) = delete;
        RenderHandle& operator=(const RenderHandle&) = delete;

        /// Formula width in pixels.
        [[nodiscard]] int width() const;
        /// Formula height above and below the baseline in pixels.
        [[nodiscard]] int height() const;
        /// Formula depth below the baseline in pixels.
        [[nodiscard]] int depth() const;
        /// Baseline offset in pixels from the top of the formula box.
        [[nodiscard]] float baseline() const;

        /// Change the formula foreground color as 0xAARRGGBB.
        ///
        /// This updates the laid-out formula's color without reparsing it.
        void set_foreground(uint32_t argb) const;
        /// Draw the formula at the given top-left pixel position.
        ///
        /// The target surface decides whether coordinates are clipped, blended,
        /// or forwarded to a GPU API.
        void draw(int x, int y) const;

    private:
        friend class LatexDocument;

        RenderHandle(tex::TeXRender* render, std::shared_ptr<detail::DocumentState> state);

        // Declaration order matters: render_ is declared last so it is destroyed
        // first, before state_ drops its reference.
        std::shared_ptr<detail::DocumentState> state_;
        std::unique_ptr<tex::TeXRender> render_;
    };
} // namespace TeXRender
