if (EMSCRIPTEN)
    set(PLATFORM Web CACHE STRING "raylib platform" FORCE)
    set(GRAPHICS GRAPHICS_API_OPENGL_ES2 CACHE STRING "raylib graphics API" FORCE)
endif ()

if (NOT EMSCRIPTEN)
    find_package(raylib CONFIG QUIET)
endif ()
if (NOT raylib_FOUND)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    texrender_add_submodule_dependency(
            raylib
            RAYLIB
            "${CMAKE_CURRENT_LIST_DIR}/../../third_party/raylib"
            CMakeLists.txt
    )
endif ()

if (TARGET raylib)
    set(Raylib_FOUND TRUE)
else ()
    set(Raylib_FOUND FALSE)
endif ()
