#include <string_view>
#include <texrender/config.hpp>
#include <texrender/texrender.hpp>

#ifndef TEXRENDER_EXPECT_BACKEND
#error "TEXRENDER_EXPECT_BACKEND must be defined"
#endif

static_assert(TEXRENDER_HAS_HEADLESS == 1);
static_assert(TEXRENDER_HAS_SVG == 1);
static_assert(TEXRENDER_HAS_RAYLIB == TEXRENDER_EXPECT_HAS_RAYLIB);
static_assert(TEXRENDER_HAS_SDL2 == TEXRENDER_EXPECT_HAS_SDL2);
static_assert(TeXRender::has_headless);
static_assert(TeXRender::has_svg);
static_assert(TeXRender::has_raylib == static_cast<bool>(TEXRENDER_EXPECT_HAS_RAYLIB));
static_assert(TeXRender::has_sdl2 == static_cast<bool>(TEXRENDER_EXPECT_HAS_SDL2));

int main()
{
    return std::string_view{TeXRender::build_backend} == TEXRENDER_EXPECT_BACKEND ? 0 : 1;
}
