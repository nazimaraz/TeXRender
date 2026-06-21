#include <type_traits>
#include <utility>
#include <texrender/raylib_surface.hpp>

static_assert(!std::is_copy_constructible_v<TeXRender::RaylibSurface>);
static_assert(!std::is_move_constructible_v<TeXRender::RaylibSurface>);
static_assert(std::is_nothrow_destructible_v<TeXRender::RaylibSurface>);
static_assert(noexcept(std::declval<TeXRender::RaylibSurface&>().unload()));
static_assert(noexcept(std::declval<const TeXRender::RaylibSurface&>().max_textures()));

int main()
{
    return 0;
}
