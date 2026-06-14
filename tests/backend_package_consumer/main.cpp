#include <type_traits>

#if defined(TEXRENDER_BACKEND_PACKAGE_RAYLIB)
#include <texrender/raylib_surface.hpp>

static_assert(!std::is_copy_constructible_v<TeXRender::RaylibSurface>);
static_assert(!std::is_move_constructible_v<TeXRender::RaylibSurface>);

int main()
{
    return sizeof(TeXRender::RaylibSurface) > 0 ? 0 : 1;
}
#elif defined(TEXRENDER_BACKEND_PACKAGE_SDL2)
#include <texrender/sdl2_surface.hpp>

static_assert(!std::is_copy_constructible_v<TeXRender::Sdl2Surface>);
static_assert(!std::is_move_constructible_v<TeXRender::Sdl2Surface>);

int main()
{
    TeXRender::set_default_sdl_renderer(nullptr);
    return TeXRender::default_sdl_renderer() == nullptr ? 0 : 1;
}
#else
#error "Select TEXRENDER_BACKEND_PACKAGE_RAYLIB or TEXRENDER_BACKEND_PACKAGE_SDL2"
#endif
