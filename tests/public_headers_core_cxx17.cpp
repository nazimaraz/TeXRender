#include <memory>
#include <type_traits>
#include <utility>
#include <texrender/cache_options.hpp>
#include <texrender/config.hpp>
#include <texrender/draw_surface.hpp>
#include <texrender/export.hpp>
#include <texrender/formula_layout.hpp>
#include <texrender/glyph_bitmap.hpp>
#include <texrender/headless_surface.hpp>
#include <texrender/latex_document.hpp>
#include <texrender/render_handle.hpp>
#include <texrender/svg_surface.hpp>
#include <texrender/texrender.hpp>
#include <texrender/version.hpp>

static_assert(TeXRender::version_major == TEXRENDER_VERSION_MAJOR);
static_assert(TEXRENDER_HAS_HEADLESS == 1);
static_assert(TEXRENDER_HAS_SVG == 1);
static_assert(TeXRender::has_headless);
static_assert(TeXRender::has_svg);
static_assert(std::is_nothrow_destructible_v<TeXRender::LatexDocument>);
static_assert(std::is_nothrow_destructible_v<TeXRender::RenderHandle>);
static_assert(std::is_move_constructible_v<TeXRender::RenderHandle>);
static_assert(std::is_nothrow_move_constructible_v<TeXRender::RenderHandle>);
static_assert(std::is_nothrow_move_assignable_v<TeXRender::RenderHandle>);
static_assert(!std::is_copy_constructible_v<TeXRender::RenderHandle>);
static_assert(!std::is_copy_constructible_v<TeXRender::LatexDocument>);
static_assert(!std::is_move_constructible_v<TeXRender::LatexDocument>);
static_assert(noexcept(std::declval<const TeXRender::RenderHandle&>().width()));
static_assert(noexcept(std::declval<const TeXRender::RenderHandle&>().height()));
static_assert(noexcept(std::declval<const TeXRender::RenderHandle&>().depth()));
static_assert(noexcept(std::declval<const TeXRender::RenderHandle&>().baseline()));
static_assert(noexcept(std::declval<const TeXRender::HeadlessSurface&>().width()));
static_assert(noexcept(std::declval<const TeXRender::HeadlessSurface&>().height()));
static_assert(noexcept(std::declval<const TeXRender::HeadlessSurface&>().pixels()));
static_assert(noexcept(std::declval<const TeXRender::HeadlessSurface&>().covered_pixels()));
static_assert(noexcept(std::declval<const TeXRender::SvgSurface&>().width()));
static_assert(noexcept(std::declval<const TeXRender::SvgSurface&>().height()));
static_assert(noexcept(std::declval<const TeXRender::SvgSurface&>().element_count()));
static_assert(std::is_same_v<decltype(*std::declval<TeXRender::RenderResult&>()), TeXRender::RenderHandle&>);
static_assert(std::is_same_v<decltype(*std::declval<const TeXRender::RenderResult&>()), const TeXRender::RenderHandle&>);
static_assert(std::is_same_v<decltype(*std::declval<TeXRender::RenderResult&&>()), TeXRender::RenderHandle&&>);
static_assert(std::is_same_v<decltype(std::declval<TeXRender::RenderResult&>().operator->()), TeXRender::RenderHandle*>);
static_assert(
    std::is_same_v<decltype(std::declval<const TeXRender::RenderResult&>().operator->()), const TeXRender::RenderHandle*>
);

int main()
{
    auto options = TeXRender::RenderOptions{};
    options.text_size = 24.f;
    options.argb = 0xff000000;

    auto error = TeXRender::RenderError{};
    error.code = TeXRender::RenderErrorCode::parse_error;
    error.message = "parse error";

    const auto result = TeXRender::RenderResult{std::move(error)};
    auto canvas = TeXRender::HeadlessSurface{};
    canvas.resize(2, 2);
    canvas.clear(0x00000000);

    auto svg = TeXRender::SvgSurface{2, 2};
    svg.clear(0xffffffff);

    auto limits = TeXRender::CacheLimits{};
    limits.max_cpu_glyphs = 16;
    limits.max_gpu_textures = 16;

    return result.has_value() || canvas.width() != 2 || svg.width() != 2 || options.text_size != 24.f || limits.max_cpu_glyphs != 16
               ? 1
               : 0;
}
