#include "core/document_state.hpp"
#include <mutex>
#include "latex.h"
#include "render/glyph_cache.hpp"
#include "render/surface_factory.hpp"

namespace TeXRender::detail
{
    namespace
    {
        std::mutex g_mutex;
        bool g_initialised = false;
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
    }

    DocumentState::~DocumentState() = default;

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
