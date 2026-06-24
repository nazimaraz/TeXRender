set(TEXRENDER_MICROTEX_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../third_party/microtex" CACHE PATH
        "Path to the MicroTeX source checkout used by texrender")
set(_texrender_microtex_source_dir "${TEXRENDER_MICROTEX_SOURCE_DIR}")
if (NOT IS_ABSOLUTE "${_texrender_microtex_source_dir}")
    get_filename_component(_texrender_microtex_source_dir "${_texrender_microtex_source_dir}" ABSOLUTE
            BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()

if (NOT EXISTS "${_texrender_microtex_source_dir}/CMakeLists.txt"
        OR NOT EXISTS "${_texrender_microtex_source_dir}/src/latex.cpp"
        OR NOT EXISTS "${_texrender_microtex_source_dir}/res/.clatexmath-res_root")
    set(MicroTeX_FOUND FALSE)
    return()
endif ()

option(TEXRENDER_MICROTEX_LOGS "Enable MicroTeX runtime debug logs" OFF)
set(HAVE_LOG "${TEXRENDER_MICROTEX_LOGS}" CACHE BOOL "Enable MicroTeX runtime debug logs" FORCE)
set(MICROTEX_OUTPUT_NAME microtex_core CACHE STRING "MicroTeX library output name")
add_subdirectory("${_texrender_microtex_source_dir}" "${CMAKE_CURRENT_BINARY_DIR}/_deps/microtex-build" EXCLUDE_FROM_ALL)
set(MICROTEX_TARGET LaTeX CACHE INTERNAL "MicroTeX CMake target used by texrender")
set(MICROTEX_SRC "${_texrender_microtex_source_dir}/src" CACHE INTERNAL "MicroTeX source directory")
set(MICROTEX_RES_DIR "${_texrender_microtex_source_dir}/res" CACHE INTERNAL "MicroTeX resource directory")
if (NOT TARGET ${MICROTEX_TARGET})
    set(MicroTeX_FOUND FALSE)
    return()
endif ()

set(MicroTeX_FOUND TRUE)
