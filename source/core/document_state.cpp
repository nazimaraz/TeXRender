#include "core/document_state.hpp"
#include <format>
#include <mutex>
#include "latex.h"
#include "render/glyph_cache.hpp"
#include "render/surface_factory.hpp"

namespace TexGui::detail
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
            debug_log(std::format("ignoring resource root '{}'; MicroTeX is already initialised with '{}'", res_root, g_res_root));
        }

        set_default_font_path(std::format("{}/fonts/latin/cmr10.ttf", g_res_root));
    }

    DocumentState::~DocumentState() = default;

    IDrawSurface* DocumentState::surface() const
    {
        return surface_.get();
    }
} // namespace TexGui::detail
