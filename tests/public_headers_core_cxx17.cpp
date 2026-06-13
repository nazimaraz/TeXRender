#include <memory>
#include <type_traits>
#include <utility>
#include <texgui/cache_options.hpp>
#include <texgui/draw_surface.hpp>
#include <texgui/export.hpp>
#include <texgui/formula_layout.hpp>
#include <texgui/glyph_bitmap.hpp>
#include <texgui/headless_surface.hpp>
#include <texgui/latex_document.hpp>
#include <texgui/render_handle.hpp>
#include <texgui/svg_surface.hpp>
#include <texgui/texgui.hpp>
#include <texgui/version.hpp>

static_assert(TexGui::version_major == TEXGUI_VERSION_MAJOR);
static_assert(std::is_move_constructible_v<TexGui::RenderHandle>);
static_assert(!std::is_copy_constructible_v<TexGui::RenderHandle>);
static_assert(!std::is_copy_constructible_v<TexGui::LatexDocument>);

int main()
{
    auto options = TexGui::RenderOptions{};
    options.text_size = 24.f;
    options.argb = 0xff000000;

    auto error = TexGui::RenderError{};
    error.code = TexGui::RenderErrorCode::parse_error;
    error.message = "parse error";

    const auto result = TexGui::RenderResult{std::move(error)};
    auto canvas = TexGui::HeadlessSurface{};
    canvas.resize(2, 2);
    canvas.clear(0x00000000);

    auto svg = TexGui::SvgSurface{2, 2};
    svg.clear(0xffffffff);

    auto limits = TexGui::CacheLimits{};
    limits.max_cpu_glyphs = 16;
    limits.max_gpu_textures = 16;

    return result.has_value() || canvas.width() != 2 || svg.width() != 2 || options.text_size != 24.f || limits.max_cpu_glyphs != 16
               ? 1
               : 0;
}
