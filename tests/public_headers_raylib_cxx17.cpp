#include <type_traits>
#include <texgui/raylib_surface.hpp>

static_assert(!std::is_copy_constructible_v<TexGui::RaylibSurface>);

int main()
{
    return 0;
}
