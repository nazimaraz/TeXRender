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

    // A move-only handle to one parsed and laid-out formula. It shares ownership
    // of the producing document's runtime state, so it stays safe to draw and to
    // destroy even if the LatexDocument is gone first.
    class RenderHandle
    {
    public:
        ~RenderHandle();

        RenderHandle(RenderHandle&&) noexcept;
        RenderHandle& operator=(RenderHandle&&) noexcept;
        RenderHandle(const RenderHandle&) = delete;
        RenderHandle& operator=(const RenderHandle&) = delete;

        [[nodiscard]] int width() const;
        [[nodiscard]] int height() const;
        [[nodiscard]] int depth() const;
        [[nodiscard]] float baseline() const;

        void set_foreground(uint32_t argb) const;
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
