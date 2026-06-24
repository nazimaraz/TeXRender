cmake_minimum_required(VERSION 3.21)
project(TeXRender VERSION 0.1.0 DESCRIPTION "Render LaTeX formulas via MicroTeX with pluggable C++ rendering backends" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(TEXRENDER_BACKEND "headless" CACHE STRING "Rendering backend to build (headless, svg, raylib, sdl2)")
set_property(CACHE TEXRENDER_BACKEND PROPERTY STRINGS raylib headless svg sdl2)
option(TEXRENDER_BUILD_EXAMPLES "Build the example applications" ${PROJECT_IS_TOP_LEVEL})
option(TEXRENDER_BUILD_TESTS "Build the unit tests" ${PROJECT_IS_TOP_LEVEL})
option(TEXRENDER_USE_SUBMODULE_DEPENDENCIES "Use third_party dependency submodules when packages are not installed" ON)
include(cmake/Dependencies.cmake)
set(TEXRENDER_RESOURCE_DIR "${MICROTEX_RES_DIR}" CACHE INTERNAL "texrender MicroTeX resource directory")
include(cmake/texrenderHelpers.cmake)
if (TEXRENDER_BACKEND STREQUAL "raylib")
    set(TEXRENDER_BACKEND_SOURCES
            source/backend/raylib/raylib_surface.cpp
            source/backend/raylib/raylib_factory.cpp
    )
    set(TEXRENDER_BACKEND_LIBS raylib)
elseif (TEXRENDER_BACKEND STREQUAL "headless")
    set(TEXRENDER_BACKEND_SOURCES source/backend/headless/headless_factory.cpp)
    set(TEXRENDER_BACKEND_LIBS "")
elseif (TEXRENDER_BACKEND STREQUAL "svg")
    set(TEXRENDER_BACKEND_SOURCES source/backend/svg/svg_factory.cpp)
    set(TEXRENDER_BACKEND_LIBS "")
elseif (TEXRENDER_BACKEND STREQUAL "sdl2")
    set(TEXRENDER_BACKEND_SOURCES
            source/backend/sdl2/sdl2_surface.cpp
            source/backend/sdl2/sdl2_factory.cpp
    )
    set(TEXRENDER_BACKEND_LIBS "${TEXRENDER_SDL2_TARGET}")
    set(TEXRENDER_BACKEND_EXECUTABLE_LIBS "${TEXRENDER_SDL2MAIN_TARGET}")
else ()
    message(FATAL_ERROR "Unsupported TEXRENDER_BACKEND='${TEXRENDER_BACKEND}'. Supported: 'raylib', 'headless', 'svg', 'sdl2'.")
endif ()

add_library(texrender
        source/core/latex_document.cpp
        source/core/cache_options.cpp
        source/core/render_handle.cpp
        source/core/document_state.cpp
        source/render/glyph_cache.cpp
        source/render/draw_surface.cpp
        source/render/ft_font.cpp
        source/render/ft_text_layout.cpp
        source/render/graphics2d_adapter.cpp
        source/backend/svg/svg_surface.cpp
        source/backend/headless/headless_surface.cpp
        ${TEXRENDER_BACKEND_SOURCES}
)
add_library(texrender::texrender ALIAS texrender)
target_include_directories(texrender PUBLIC include)
target_include_directories(texrender PRIVATE source)
target_include_directories(texrender SYSTEM PRIVATE ${MICROTEX_SRC})
target_link_libraries(texrender PRIVATE microtex::core Freetype::Freetype)
target_link_libraries(microtex_core PRIVATE texrender)
target_link_libraries(texrender PUBLIC ${TEXRENDER_BACKEND_LIBS})
target_compile_features(texrender PRIVATE cxx_std_17)
target_compile_features(texrender INTERFACE cxx_std_17)
if (TEXRENDER_BUILD_EXAMPLES)
    set(TEXRENDER_EXAMPLES
            texrender_offscreen examples/offscreen.cpp
            texrender_headless_batch examples/headless_batch.cpp
            texrender_untrusted_worker examples/untrusted_worker.cpp
            texrender_svg_export examples/svg_export.cpp
            texrender_custom_surface examples/custom_surface.cpp
            texrender_formula_label_widget examples/formula_label_widget.cpp
    )
    if (TEXRENDER_BACKEND STREQUAL "raylib")
        list(APPEND TEXRENDER_EXAMPLES
                texrender_minimal examples/minimal.cpp
                texrender_demo examples/demo_main.cpp
                texrender_realtime_values examples/realtime_values.cpp
                texrender_live examples/live_edit.cpp
                texrender_raylib_overlay examples/raylib_overlay.cpp
        )
    elseif (TEXRENDER_BACKEND STREQUAL "sdl2")
        list(APPEND TEXRENDER_EXAMPLES texrender_sdl2_minimal examples/sdl2_minimal.cpp)
    endif ()

    while (TEXRENDER_EXAMPLES)
        list(POP_FRONT TEXRENDER_EXAMPLES example_name example_source)
        add_executable(${example_name} ${example_source})
        target_link_libraries(${example_name} PRIVATE texrender::texrender ${TEXRENDER_BACKEND_EXECUTABLE_LIBS})
        set_target_properties(${example_name} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
        texrender_copy_resources(${example_name})
    endwhile ()
endif ()
