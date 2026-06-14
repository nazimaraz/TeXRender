#pragma once

#include <memory>
#include <mutex>
#include <string>
#include "texrender/draw_surface.hpp"

namespace TeXRender::detail
{
    class DocumentState
    {
    public:
        // A null surface means "use the built backend's default" (make_surface).
        explicit DocumentState(const std::string& res_root, std::unique_ptr<IDrawSurface> surface = nullptr);
        DocumentState(const DocumentState&) = delete;
        DocumentState(DocumentState&&) = delete;
        DocumentState& operator=(const DocumentState&) = delete;
        DocumentState& operator=(DocumentState&&) = delete;
        ~DocumentState();

        [[nodiscard]] IDrawSurface* surface() const;

    private:
        std::unique_ptr<IDrawSurface> surface_;
    };

    [[nodiscard]] std::recursive_mutex& render_mutex();
} // namespace TeXRender::detail
