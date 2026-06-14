#include <type_traits>
#include <texrender/sdl2_surface.hpp>

static_assert(!std::is_copy_constructible_v<TeXRender::Sdl2Surface>);

int main()
{
    TeXRender::set_default_sdl_renderer(nullptr);
    return TeXRender::default_sdl_renderer() == nullptr ? 0 : 1;
}
