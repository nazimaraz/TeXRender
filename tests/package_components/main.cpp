#include <string_view>
#include <texgui/config.hpp>
#include <texgui/texgui.hpp>

#ifndef TEXGUI_EXPECT_BACKEND
#error "TEXGUI_EXPECT_BACKEND must be defined"
#endif

static_assert(TEXGUI_HAS_HEADLESS == 1);
static_assert(TEXGUI_HAS_SVG == 1);
static_assert(TEXGUI_HAS_RAYLIB == TEXGUI_EXPECT_HAS_RAYLIB);
static_assert(TEXGUI_HAS_SDL2 == TEXGUI_EXPECT_HAS_SDL2);
static_assert(TexGui::has_headless);
static_assert(TexGui::has_svg);
static_assert(TexGui::has_raylib == static_cast<bool>(TEXGUI_EXPECT_HAS_RAYLIB));
static_assert(TexGui::has_sdl2 == static_cast<bool>(TEXGUI_EXPECT_HAS_SDL2));

int main()
{
    return std::string_view{TexGui::build_backend} == TEXGUI_EXPECT_BACKEND ? 0 : 1;
}
