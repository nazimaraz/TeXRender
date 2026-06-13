#include <type_traits>

#if defined(TEXGUI_BACKEND_PACKAGE_RAYLIB)
#include <texgui/raylib_surface.hpp>

static_assert(!std::is_copy_constructible_v<TexGui::RaylibSurface>);
static_assert(!std::is_move_constructible_v<TexGui::RaylibSurface>);

int main()
{
    return sizeof(TexGui::RaylibSurface) > 0 ? 0 : 1;
}
#elif defined(TEXGUI_BACKEND_PACKAGE_SDL2)
#include <texgui/sdl2_surface.hpp>

static_assert(!std::is_copy_constructible_v<TexGui::Sdl2Surface>);
static_assert(!std::is_move_constructible_v<TexGui::Sdl2Surface>);

int main()
{
    TexGui::set_default_sdl_renderer(nullptr);
    return TexGui::default_sdl_renderer() == nullptr ? 0 : 1;
}
#else
#error "Select TEXGUI_BACKEND_PACKAGE_RAYLIB or TEXGUI_BACKEND_PACKAGE_SDL2"
#endif
