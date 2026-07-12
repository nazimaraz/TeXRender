cmake_minimum_required(VERSION 3.21)
project(TeXRender VERSION 0.1.0 DESCRIPTION "Render LaTeX formulas via MicroTeX with pluggable C++ rendering backends" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(TEXRENDER_BACKEND "headless" CACHE STRING "Rendering backend to build (headless, svg, raylib, sdl2)")
set_property(CACHE TEXRENDER_BACKEND PROPERTY STRINGS raylib headless svg sdl2)
set(TEXRENDER_BUILD_EXAMPLES_DEFAULT "${PROJECT_IS_TOP_LEVEL}")
set(TEXRENDER_BUILD_TESTS_DEFAULT "${PROJECT_IS_TOP_LEVEL}")
if (EMSCRIPTEN)
    set(TEXRENDER_BUILD_EXAMPLES_DEFAULT OFF)
    set(TEXRENDER_BUILD_TESTS_DEFAULT OFF)
endif ()
option(TEXRENDER_BUILD_EXAMPLES "Build the example applications" ${TEXRENDER_BUILD_EXAMPLES_DEFAULT})
option(TEXRENDER_BUILD_TESTS "Build texrender smoke tests" ${TEXRENDER_BUILD_TESTS_DEFAULT})
option(TEXRENDER_BUILD_BENCHMARKS "Build texrender benchmark executables" OFF)
unset(TEXRENDER_BUILD_EXAMPLES_DEFAULT)
unset(TEXRENDER_BUILD_TESTS_DEFAULT)
set(TEXRENDER_BUILD_WASM_EXAMPLE_DEFAULT OFF)
if (PROJECT_IS_TOP_LEVEL AND EMSCRIPTEN)
    set(TEXRENDER_BUILD_WASM_EXAMPLE_DEFAULT ON)
endif ()
option(TEXRENDER_BUILD_WASM_EXAMPLE "Build the WebAssembly browser example when using Emscripten" ${TEXRENDER_BUILD_WASM_EXAMPLE_DEFAULT})
unset(TEXRENDER_BUILD_WASM_EXAMPLE_DEFAULT)
option(TEXRENDER_USE_SUBMODULE_DEPENDENCIES "Use third_party dependency submodules when packages are not installed" ON)
include(cmake/Dependencies.cmake)
set(TEXRENDER_RESOURCE_DIR "${MICROTEX_RES_DIR}" CACHE INTERNAL "texrender MicroTeX resource directory")
include(cmake/texrenderHelpers.cmake)

function(texrender_add_surface_component component_name component_source)
    add_library(${component_name} OBJECT ${component_source})
    target_include_directories(${component_name} PUBLIC include)
    target_include_directories(${component_name} PRIVATE source)
    target_compile_features(${component_name} PRIVATE cxx_std_17)
endfunction()

texrender_add_surface_component(texrender_headless_surface source/backend/headless/headless_surface.cpp)
texrender_add_surface_component(texrender_svg_surface source/backend/svg/svg_surface.cpp)
add_library(texrender::headless_surface ALIAS texrender_headless_surface)
add_library(texrender::svg_surface ALIAS texrender_svg_surface)

function(texrender_link_surface_component target_name component_backend)
    if (NOT TEXRENDER_BACKEND STREQUAL component_backend)
        target_link_libraries(${target_name} PRIVATE texrender_${component_backend}_surface)
    endif ()
endfunction()

set(TEXRENDER_BACKEND_OBJECTS "")
if (TEXRENDER_BACKEND STREQUAL "raylib")
    set(TEXRENDER_BACKEND_SOURCES
            source/backend/raylib/raylib_surface.cpp
            source/backend/raylib/raylib_factory.cpp
    )
    set(TEXRENDER_BACKEND_LIBS raylib)
elseif (TEXRENDER_BACKEND STREQUAL "headless")
    set(TEXRENDER_BACKEND_SOURCES source/backend/headless/headless_factory.cpp)
    list(APPEND TEXRENDER_BACKEND_OBJECTS $<TARGET_OBJECTS:texrender_headless_surface>)
    set(TEXRENDER_BACKEND_LIBS "")
elseif (TEXRENDER_BACKEND STREQUAL "svg")
    set(TEXRENDER_BACKEND_SOURCES source/backend/svg/svg_factory.cpp)
    list(APPEND TEXRENDER_BACKEND_OBJECTS $<TARGET_OBJECTS:texrender_svg_surface>)
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
        ${TEXRENDER_BACKEND_SOURCES}
        ${TEXRENDER_BACKEND_OBJECTS}
)
add_library(texrender::texrender ALIAS texrender)
set_target_properties(texrender PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
)
target_include_directories(texrender PUBLIC include)
target_include_directories(texrender PRIVATE source)
target_include_directories(texrender SYSTEM PRIVATE ${MICROTEX_SRC})
target_link_libraries(texrender PRIVATE ${MICROTEX_TARGET} Freetype::Freetype)
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
                live_edit examples/live_edit.cpp
                texrender_raylib_overlay examples/raylib_overlay.cpp
        )
    elseif (TEXRENDER_BACKEND STREQUAL "sdl2")
        list(APPEND TEXRENDER_EXAMPLES texrender_sdl2_minimal examples/sdl2_minimal.cpp)
    endif ()

    while (TEXRENDER_EXAMPLES)
        list(POP_FRONT TEXRENDER_EXAMPLES example_name example_source)
        add_executable(${example_name} ${example_source})
        target_link_libraries(${example_name} PRIVATE texrender::texrender ${TEXRENDER_BACKEND_EXECUTABLE_LIBS})
        if (example_name MATCHES "^(texrender_offscreen|texrender_headless_batch|texrender_untrusted_worker|texrender_formula_label_widget)$")
            texrender_link_surface_component(${example_name} headless)
        elseif (example_name STREQUAL "texrender_svg_export")
            texrender_link_surface_component(${example_name} svg)
        endif ()
        set_target_properties(${example_name} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
        texrender_copy_resources(${example_name})
    endwhile ()
endif ()

if (TEXRENDER_BUILD_WASM_EXAMPLE)
    if (NOT EMSCRIPTEN)
        message(FATAL_ERROR "TEXRENDER_BUILD_WASM_EXAMPLE requires the Emscripten toolchain")
    endif ()

    set(TEXRENDER_WASM_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/wasm")
    add_executable(texrender_wasm examples/wasm/texrender_wasm.cpp)
    target_link_libraries(texrender_wasm PRIVATE texrender::texrender)
    texrender_link_surface_component(texrender_wasm svg)
    set_target_properties(texrender_wasm PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
            RUNTIME_OUTPUT_DIRECTORY "${TEXRENDER_WASM_OUTPUT_DIR}"
            SUFFIX ".js"
    )
    target_link_options(texrender_wasm PRIVATE
            "--no-entry"
            "--preload-file" "${TEXRENDER_RESOURCE_DIR}@/texrender-res"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sFILESYSTEM=1"
            "-sMODULARIZE=1"
            "-sEXPORT_NAME=createTeXRenderModule"
            "-sEXPORTED_FUNCTIONS=['_texrender_render_svg','_texrender_last_error']"
            "-sEXPORTED_RUNTIME_METHODS=['cwrap']"
    )
    add_custom_command(TARGET texrender_wasm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/examples/wasm/index.html"
                    "${TEXRENDER_WASM_OUTPUT_DIR}/index.html"
            COMMENT "Copying texrender WebAssembly example page"
            VERBATIM
    )

    if (TEXRENDER_BACKEND STREQUAL "raylib")
        set(TEXRENDER_WASM_RAYLIB_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/wasm-raylib")
        add_executable(texrender_wasm_raylib examples/wasm/raylib_wasm.cpp)
        target_link_libraries(texrender_wasm_raylib PRIVATE texrender::texrender)
        target_compile_definitions(texrender_wasm_raylib PRIVATE "TEXRENDER_RES_DIR=\"/texrender-res\"")
        set_target_properties(texrender_wasm_raylib PROPERTIES
                CXX_STANDARD 17
                CXX_STANDARD_REQUIRED ON
                RUNTIME_OUTPUT_DIRECTORY "${TEXRENDER_WASM_RAYLIB_OUTPUT_DIR}"
                SUFFIX ".js"
        )
        target_link_options(texrender_wasm_raylib PRIVATE
                "--preload-file" "${TEXRENDER_RESOURCE_DIR}@/texrender-res"
                "-sALLOW_MEMORY_GROWTH=1"
                "-sFILESYSTEM=1"
                "-sMODULARIZE=1"
                "-sEXPORT_NAME=createTeXRenderRaylibModule"
                "-sEXPORTED_FUNCTIONS=['_main','_texrender_raylib_render','_texrender_raylib_last_error','_texrender_raylib_width','_texrender_raylib_height']"
                "-sEXPORTED_RUNTIME_METHODS=['cwrap']"
                "-sASYNCIFY"
        )
        add_custom_command(TARGET texrender_wasm_raylib POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${CMAKE_CURRENT_SOURCE_DIR}/examples/wasm/raylib.html"
                        "${TEXRENDER_WASM_RAYLIB_OUTPUT_DIR}/index.html"
                COMMENT "Copying texrender raylib WebAssembly example page"
                VERBATIM
        )
    endif ()
endif ()

if (TEXRENDER_BUILD_TESTS)
    include(CTest)
    if (BUILD_TESTING)
        function(texrender_add_test_executable target_name source_file)
            add_executable(${target_name} ${source_file})
            target_include_directories(${target_name} PRIVATE tests)
            target_link_libraries(${target_name} PRIVATE texrender::texrender)
            texrender_link_surface_component(${target_name} headless)
            texrender_link_surface_component(${target_name} svg)
            set_target_properties(${target_name} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
            texrender_copy_resources(${target_name})
            add_test(NAME ${target_name} COMMAND ${target_name})
        endfunction()

        texrender_add_test_executable(texrender_smoke_test tests/render_smoke.cpp)
        texrender_add_test_executable(texrender_surface_tests tests/surface_tests.cpp)
        texrender_add_test_executable(texrender_render_options_tests tests/render_options_tests.cpp)
    endif ()
endif ()

if (TEXRENDER_BUILD_BENCHMARKS)
    add_executable(texrender_benchmark benchmarks/render_benchmark.cpp)
    target_link_libraries(texrender_benchmark PRIVATE texrender::texrender)
    texrender_link_surface_component(texrender_benchmark headless)
    texrender_link_surface_component(texrender_benchmark svg)
    set_target_properties(texrender_benchmark PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
    texrender_copy_resources(texrender_benchmark)
endif ()
