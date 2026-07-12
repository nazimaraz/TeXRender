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
        explicit DocumentState(const std::string& res_root, std::unique_ptr<IDrawSurface> surface = nullptr);
        DocumentState(const DocumentState&) = delete;
        DocumentState(DocumentState&&) = delete;
        DocumentState& operator=(const DocumentState&) = delete;
        DocumentState& operator=(DocumentState&&) = delete;
        ~DocumentState();

        [[nodiscard]] IDrawSurface* surface() const;
        [[nodiscard]] std::mutex& surface_mutex() noexcept;

    private:
        std::unique_ptr<IDrawSurface> surface_;
        std::mutex surface_mutex_;
    };

    [[nodiscard]] std::recursive_mutex& render_mutex();
    void register_microtex_platform_factory();
} // namespace TeXRender::detail
