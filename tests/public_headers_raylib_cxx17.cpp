#include <type_traits>
#include <texrender/raylib_surface.hpp>

static_assert(!std::is_copy_constructible_v<TeXRender::RaylibSurface>);

int main()
{
    return 0;
}
