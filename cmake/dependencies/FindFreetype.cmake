if (NOT EMSCRIPTEN)
    set(_texrender_freetype_find_required "${Freetype_FIND_REQUIRED}")
    set(Freetype_FIND_REQUIRED FALSE)
    include("${CMAKE_ROOT}/Modules/FindFreetype.cmake")
    set(Freetype_FIND_REQUIRED "${_texrender_freetype_find_required}")
    unset(_texrender_freetype_find_required)
endif ()
if (NOT Freetype_FOUND)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    texrender_add_submodule_dependency(
            freetype
            FREETYPE
            "${CMAKE_CURRENT_LIST_DIR}/../../third_party/freetype"
            CMakeLists.txt
    )
    unset(CMAKE_POLICY_VERSION_MINIMUM)
    add_library(Freetype::Freetype ALIAS freetype)
endif ()

if (TARGET Freetype::Freetype)
    set(Freetype_FOUND TRUE)
else ()
    set(Freetype_FOUND FALSE)
endif ()
