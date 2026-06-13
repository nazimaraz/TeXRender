#include <type_traits>
#include <texgui/sdl2_surface.hpp>

static_assert(!std::is_copy_constructible_v<TexGui::Sdl2Surface>);

int main()
{
    TexGui::set_default_sdl_renderer(nullptr);
    return TexGui::default_sdl_renderer() == nullptr ? 0 : 1;
}
