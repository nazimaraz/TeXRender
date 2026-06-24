#include "document_state.hpp"
#include <cstddef>
#include <mutex>
#include "latex.h"
#include "render/glyph_cache.hpp"
#include "render/surface_factory.hpp"
#include "texrender/latex_document.hpp"

namespace TeXRender::detail
{
    namespace
    {
        std::mutex g_mutex;
        bool g_initialised = false;
        std::size_t g_active_states = 0;
        std::string g_res_root;
    } // namespace

    DocumentState::DocumentState(const std::string& res_root, std::unique_ptr<IDrawSurface> surface)
        : surface_{surface ? std::move(surface) : make_surface()}
    {
        const auto lock = std::scoped_lock{g_mutex};
        if (!g_initialised)
        {
            tex::LaTeX::init(res_root);
            g_res_root = res_root;
            g_initialised = true;
        }
        else if (res_root != g_res_root)
        {
            debug_log("ignoring resource root '" + res_root + "'; MicroTeX is already initialised with '" + g_res_root + "'");
        }

        set_default_font_path(g_res_root + "/fonts/latin/cmr10.ttf");
        ++g_active_states;
    }

    DocumentState::~DocumentState()
    {
        const auto lock = std::scoped_lock{g_mutex};
        if (g_active_states > 0)
            --g_active_states;
    }

    IDrawSurface* DocumentState::surface() const
    {
        return surface_.get();
    }

    std::recursive_mutex& render_mutex()
    {
        static auto mutex = std::recursive_mutex{};
        return mutex;
    }
} // namespace TeXRender::detail

void TeXRender::shutdown() noexcept
{
    using namespace TeXRender::detail;

    const auto lock = std::scoped_lock{render_mutex(), g_mutex};
    if (!g_initialised)
        return;

    if (g_active_states > 0)
    {
        debug_log("shutdown ignored while LatexDocument or RenderHandle instances are still alive");
        return;
    }

    debug_log("shutdown is currently a no-op; MicroTeX state remains process-lifetime");
}
