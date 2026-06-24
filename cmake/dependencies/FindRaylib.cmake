find_package(raylib CONFIG QUIET)
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
