#pragma once

#include <cstdint>
#include <memory>

namespace tex
{
    class TeXRender;
}

namespace TexGui
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
    class RenderHandle
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
        void set_foreground(uint32_t argb) const;
        /// Draw the formula at the given top-left pixel position.
        void draw(int x, int y) const;

    private:
        friend class LatexDocument;

        RenderHandle(tex::TeXRender* render, std::shared_ptr<detail::DocumentState> state);

        // Declaration order matters: render_ is declared last so it is destroyed
        // first, before state_ drops its reference.
        std::shared_ptr<detail::DocumentState> state_;
        std::unique_ptr<tex::TeXRender> render_;
    };
} // namespace TexGui
