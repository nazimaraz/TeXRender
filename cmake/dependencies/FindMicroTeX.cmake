set(_texrender_default_microtex_source_dir "${CMAKE_CURRENT_LIST_DIR}/../../third_party/microtex")
set(TEXRENDER_MICROTEX_SOURCE_DIR "${_texrender_default_microtex_source_dir}" CACHE PATH
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

set(MICROTEX_BUILD_PLATFORM_BACKENDS OFF CACHE BOOL "" FORCE)
set(MICROTEX_OUTPUT_NAME microtex_core CACHE STRING "" FORCE)
set(BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
add_subdirectory("${_texrender_microtex_source_dir}" "${CMAKE_CURRENT_BINARY_DIR}/_deps/microtex-build" EXCLUDE_FROM_ALL)
set(MICROTEX_SRC "${_texrender_microtex_source_dir}/src" CACHE INTERNAL "MicroTeX source directory")
set(MICROTEX_RES_DIR "${_texrender_microtex_source_dir}/res" CACHE INTERNAL "MicroTeX resource directory")
set(MICROTEX_TARGET LaTeX CACHE INTERNAL "MicroTeX CMake target used by texrender")
if (TARGET microtex::core)
    set(MicroTeX_FOUND TRUE)
else ()
    set(MicroTeX_FOUND FALSE)
endif ()
