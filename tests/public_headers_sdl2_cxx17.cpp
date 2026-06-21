#include <type_traits>
#include <utility>
#include <texrender/sdl2_surface.hpp>

static_assert(!std::is_copy_constructible_v<TeXRender::Sdl2Surface>);
static_assert(!std::is_move_constructible_v<TeXRender::Sdl2Surface>);
static_assert(std::is_nothrow_destructible_v<TeXRender::Sdl2Surface>);
static_assert(noexcept(std::declval<TeXRender::Sdl2Surface&>().unload()));
static_assert(noexcept(std::declval<const TeXRender::Sdl2Surface&>().max_textures()));

int main()
{
    TeXRender::set_default_sdl_renderer(nullptr);
    return TeXRender::default_sdl_renderer() == nullptr ? 0 : 1;
}
