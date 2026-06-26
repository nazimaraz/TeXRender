#pragma once

#include <cstdint>
#include <memory>
#include "texrender/export.hpp"

namespace microtex
{
    class Render;
}

namespace TeXRender
{
    class LatexDocument;

    namespace detail
    {
        class DocumentState;
    }

    class RenderHandle
    {
        friend class LatexDocument;

    public:
        TEXRENDER_EXPORT ~RenderHandle() noexcept;
        TEXRENDER_EXPORT RenderHandle(RenderHandle&&) noexcept;
        TEXRENDER_EXPORT RenderHandle& operator=(RenderHandle&& other) noexcept;
        RenderHandle(const RenderHandle&) = delete;
        RenderHandle& operator=(const RenderHandle&) = delete;

        [[nodiscard]] TEXRENDER_EXPORT int width() const noexcept;
        [[nodiscard]] TEXRENDER_EXPORT int height() const noexcept;
        [[nodiscard]] TEXRENDER_EXPORT int depth() const noexcept;
        [[nodiscard]] TEXRENDER_EXPORT float baseline() const noexcept;
        TEXRENDER_EXPORT void set_foreground(std::uint32_t argb) const;
        TEXRENDER_EXPORT void draw(int x, int y) const;

    private:
        RenderHandle(microtex::Render* render, std::shared_ptr<detail::DocumentState> state);

        std::shared_ptr<detail::DocumentState> state_;
        std::unique_ptr<microtex::Render> render_;
    };
} // namespace TeXRender
