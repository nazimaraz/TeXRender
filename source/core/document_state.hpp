#pragma once

#include <memory>
#include <string>
#include "texgui/draw_surface.hpp"

namespace TexGui::detail
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
} // namespace TexGui::detail
