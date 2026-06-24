set(TEXRENDER_SDL2_TARGET "" CACHE INTERNAL "SDL2 target used by texrender" FORCE)
set(TEXRENDER_SDL2MAIN_TARGET "" CACHE INTERNAL "Optional SDL2main target used by SDL2 examples" FORCE)
set(_texrender_sdl2_find_required "${SDL2_FIND_REQUIRED}")
set(SDL2_FIND_REQUIRED FALSE)
find_package(SDL2 CONFIG QUIET)
if (NOT TARGET SDL2::SDL2 AND EXISTS "${CMAKE_ROOT}/Modules/FindSDL2.cmake")
    include("${CMAKE_ROOT}/Modules/FindSDL2.cmake")
endif ()
set(SDL2_FIND_REQUIRED "${_texrender_sdl2_find_required}")
unset(_texrender_sdl2_find_required)

if (NOT TARGET SDL2::SDL2 AND NOT TARGET SDL2::SDL2-static AND NOT TARGET SDL2)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    texrender_add_submodule_dependency(
            SDL2
            SDL2
            "${CMAKE_CURRENT_LIST_DIR}/../../third_party/sdl2"
            CMakeLists.txt
    )
endif ()

if (TARGET SDL2::SDL2)
    set(TEXRENDER_SDL2_TARGET SDL2::SDL2 CACHE INTERNAL "SDL2 target used by texrender" FORCE)
elseif (TARGET SDL2::SDL2-static)
    set(TEXRENDER_SDL2_TARGET SDL2::SDL2-static CACHE INTERNAL "SDL2 target used by texrender" FORCE)
elseif (TARGET SDL2)
    set(TEXRENDER_SDL2_TARGET SDL2 CACHE INTERNAL "SDL2 target used by texrender" FORCE)
else ()
    message(FATAL_ERROR "TEXRENDER_BACKEND=sdl2 requires an SDL2 CMake target")
endif ()

if (TARGET SDL2::SDL2main)
    set(TEXRENDER_SDL2MAIN_TARGET SDL2::SDL2main CACHE INTERNAL "Optional SDL2main target used by SDL2 examples" FORCE)
endif ()

set(SDL2_FOUND TRUE)
