cmake_minimum_required(VERSION 3.21)

project(
    latex_renderer
    VERSION 0.1.0
    DESCRIPTION "Render LaTeX into a GUI window via MicroTeX with a pluggable rendering backend"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# The public version header is hand-written (so it is browsable in the repo);
# fail the configure if it drifts from project(VERSION) above.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/texgui/version.hpp" _texgui_version_header)
if(NOT _texgui_version_header MATCHES "#define TEXGUI_VERSION_MAJOR ${PROJECT_VERSION_MAJOR}\n"
   OR NOT _texgui_version_header MATCHES "#define TEXGUI_VERSION_MINOR ${PROJECT_VERSION_MINOR}\n"
   OR NOT _texgui_version_header MATCHES "#define TEXGUI_VERSION_PATCH ${PROJECT_VERSION_PATCH}\n")
    message(FATAL_ERROR
        "include/texgui/version.hpp is out of sync with project(VERSION ${PROJECT_VERSION}).")
endif()
unset(_texgui_version_header)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
set(TEXGUI_BACKEND "raylib" CACHE STRING "GUI rendering backend to build (raylib, headless)")
set_property(CACHE TEXGUI_BACKEND PROPERTY STRINGS raylib headless)
option(TEXGUI_BUILD_EXAMPLES "Build the example applications" ${PROJECT_IS_TOP_LEVEL})
option(TEXGUI_BUILD_TESTS "Build the unit tests" ${PROJECT_IS_TOP_LEVEL})
option(TEXGUI_BUILD_BENCHMARKS "Build the benchmark executables" ${PROJECT_IS_TOP_LEVEL})
option(TEXGUI_BUILD_FUZZERS "Build libFuzzer targets (Clang/libFuzzer)" OFF)

# Default to Release, but only when texgui is the top-level project: a
# dependency must not force a build type into the consuming project's cache.
if(PROJECT_IS_TOP_LEVEL AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# WebAssembly (Emscripten) disables C++ exceptions by default, but MicroTeX
# reports parse errors by throwing (render()/try_render() depend on catching
# them). Build everything — dependencies included, hence before they are added
# — with native wasm exception handling.
if(EMSCRIPTEN)
    add_compile_options(-fwasm-exceptions)
    add_link_options(-fwasm-exceptions)
endif()

# ---------------------------------------------------------------------------
# Dependencies: MicroTeX core, FreeType, tinyxml2, and the selected backend.
# ---------------------------------------------------------------------------
include(cmake/Dependencies.cmake)

set(TEXGUI_RESOURCE_DIR "${MICROTEX_RES_DIR}" CACHE INTERNAL "texgui MicroTeX resource directory")
include(cmake/texguiHelpers.cmake)

# ---------------------------------------------------------------------------
# Backend selection (Open/Closed: add a new backend => add a branch + sources)
# ---------------------------------------------------------------------------
# TEXGUI_BACKEND picks which surface make_surface() creates by default (the
# *_factory.cpp that gets compiled) and which backend-specific sources/libs are
# pulled in. The windowless HeadlessSurface is dependency-free, so it ships in
# every build (see the library sources below) for offscreen rendering.
if(TEXGUI_BACKEND STREQUAL "raylib")
    set(TEXGUI_BACKEND_SOURCES
        source/backend/raylib/raylib_surface.cpp
        source/backend/raylib/raylib_factory.cpp
    )
    set(TEXGUI_BACKEND_LIBS raylib)
elseif(TEXGUI_BACKEND STREQUAL "headless")
    set(TEXGUI_BACKEND_SOURCES
        source/backend/headless/headless_factory.cpp
    )
    set(TEXGUI_BACKEND_LIBS "")
else()
    message(FATAL_ERROR
        "Unsupported TEXGUI_BACKEND='${TEXGUI_BACKEND}'. Supported: 'raylib', 'headless'.")
endif()

# ---------------------------------------------------------------------------
# The public library
# ---------------------------------------------------------------------------
add_library(texgui
    source/core/latex_document.cpp
    source/core/render_handle.cpp
    source/core/document_state.cpp
    source/render/glyph_cache.cpp
    source/render/draw_surface.cpp
    source/render/ft_font.cpp
    source/render/ft_text_layout.cpp
    source/render/graphics2d_adapter.cpp
    # The headless canvas is always available (offscreen rendering from any
    # backend); TEXGUI_BACKEND only selects the default surface.
    source/backend/headless/headless_surface.cpp
    ${TEXGUI_BACKEND_SOURCES}
)
add_library(texgui::texgui ALIAS texgui)

target_include_directories(texgui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(texgui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)
target_include_directories(texgui SYSTEM PRIVATE
    ${MICROTEX_SRC}
)

target_link_libraries(texgui
    PRIVATE microtex_core Freetype::Freetype
)
if(TEXGUI_BACKEND_LIBS)
    # The selected backend's headers are part of our public surface (e.g. the
    # user constructs a RaylibSurface), so the backend lib propagates. When
    # raylib came from FetchContent, it is only a build-tree target; installed
    # packages instead rely on find_dependency(raylib) creating that target.
    target_link_libraries(texgui PUBLIC
        "$<BUILD_INTERFACE:${TEXGUI_BACKEND_LIBS}>"
        "$<INSTALL_INTERFACE:${TEXGUI_BACKEND_LIBS}>"
    )
endif()

# texgui's own sources use C++23 (std::format/std::print), but the public
# headers compile as C++17 — that is all we impose on consumers.
target_compile_features(texgui PRIVATE cxx_std_23)
target_compile_features(texgui INTERFACE cxx_std_17)

# ---------------------------------------------------------------------------
# Example applications
# ---------------------------------------------------------------------------
if(TEXGUI_BUILD_EXAMPLES)
    if(EMSCRIPTEN AND TEXGUI_BACKEND STREQUAL "raylib")
        # Browser build: only the web-adapted live editor (a blocking
        # while-loop example would freeze the tab). It is emitted into
        # <build>/site/ as index.html + .js/.wasm/.data — a directory ready
        # for any static host (GitHub Pages).
        set(TEXGUI_EXAMPLES
            texgui_live examples/live_edit.cpp
        )
    else()
        # name -> source file. texgui_offscreen renders through an injected
        # HeadlessSurface into a PPM file, so it builds under any backend with
        # no window or GPU; the raylib examples drive a real window themselves.
        set(TEXGUI_EXAMPLES
            texgui_offscreen examples/offscreen.cpp
            texgui_headless_batch examples/headless_batch.cpp
        )
        if(TEXGUI_BACKEND STREQUAL "raylib")
            list(APPEND TEXGUI_EXAMPLES
                texgui_minimal        examples/minimal.cpp
                texgui_demo           examples/demo_main.cpp
                texgui_live           examples/live_edit.cpp
                texgui_raylib_overlay examples/raylib_overlay.cpp
            )
        endif()
    endif()
    while(TEXGUI_EXAMPLES)
        list(POP_FRONT TEXGUI_EXAMPLES example_name example_source)
        add_executable(${example_name} ${example_source})
        target_link_libraries(${example_name} PRIVATE texgui::texgui ${TEXGUI_BACKEND_LIBS})
        # Examples are consumer-shaped code: pinning them to C++17 makes every
        # build prove the public headers require nothing newer than that.
        set_target_properties(${example_name} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
        # MicroTeX needs its resource directory (fonts + metrics) at runtime.
        # On the web that directory is packed into the .data file below and
        # mounted at /res in the virtual filesystem.
        if(EMSCRIPTEN)
            target_compile_definitions(${example_name} PRIVATE TEXGUI_RES_DIR="/res")
        else()
            texgui_copy_resources(${example_name})
        endif()
    endwhile()

    if(TARGET texgui_live AND EMSCRIPTEN)
        set_target_properties(texgui_live PROPERTIES
            OUTPUT_NAME index
            SUFFIX ".html"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/site"
        )
        target_link_options(texgui_live PRIVATE
            "-sUSE_GLFW=3"                # raylib's web backend speaks GLFW
            "-sALLOW_MEMORY_GROWTH=1"
            "-sSTACK_SIZE=2097152"        # MicroTeX's parser recurses; default 64K is tight
            "SHELL:--preload-file ${MICROTEX_RES_DIR}@/res"
            "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/examples/web/shell.html"
        )
        set_property(TARGET texgui_live APPEND PROPERTY
            LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/examples/web/shell.html")
    endif()
endif()

# ---------------------------------------------------------------------------
# Benchmarks
# ---------------------------------------------------------------------------
if(TEXGUI_BUILD_BENCHMARKS AND NOT EMSCRIPTEN)
    add_executable(texgui_render_benchmark benchmarks/render_benchmark.cpp)
    target_link_libraries(texgui_render_benchmark PRIVATE texgui::texgui)
    target_compile_features(texgui_render_benchmark PRIVATE cxx_std_17)
    texgui_copy_resources(texgui_render_benchmark)
endif()

# ---------------------------------------------------------------------------
# Unit tests (not for the wasm cross-build: they read the MicroTeX res/ tree
# from the host filesystem, which the wasm sandbox cannot see)
# ---------------------------------------------------------------------------
if(TEXGUI_BUILD_TESTS AND NOT EMSCRIPTEN)
    enable_testing()

    # Pure transform math — no dependencies.
    add_executable(texgui_transform2d_test tests/transform2d_test.cpp)
    target_include_directories(texgui_transform2d_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)
    target_compile_features(texgui_transform2d_test PRIVATE cxx_std_23)
    add_test(NAME transform2d COMMAND texgui_transform2d_test)

    # FreeType glyph cache against the bundled TeX fonts — no window/GPU needed.
    add_executable(texgui_glyph_cache_test tests/glyph_cache_test.cpp)
    target_link_libraries(texgui_glyph_cache_test PRIVATE texgui::texgui)
    target_include_directories(texgui_glyph_cache_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)
    target_compile_definitions(texgui_glyph_cache_test PRIVATE TEXGUI_RES_DIR="${MICROTEX_RES_DIR}")
    add_test(NAME glyph_cache COMMAND texgui_glyph_cache_test)

    # End-to-end render through the public API into an injected headless canvas,
    # inspected pixel-by-pixel. The canvas is pure software, so this runs under
    # any backend with no window, GPU, or display.
    add_executable(texgui_headless_render_test tests/headless_render_test.cpp)
    target_link_libraries(texgui_headless_render_test PRIVATE texgui::texgui)
    target_compile_definitions(texgui_headless_render_test PRIVATE TEXGUI_RES_DIR="${MICROTEX_RES_DIR}")
    add_test(NAME headless_render COMMAND texgui_headless_render_test)

    # Structural visual regression over the software surface: stable enough for
    # CI across platforms, but stronger than "some pixels changed".
    add_executable(texgui_visual_regression_test tests/visual_regression_test.cpp)
    target_link_libraries(texgui_visual_regression_test PRIVATE texgui::texgui)
    target_compile_features(texgui_visual_regression_test PRIVATE cxx_std_23)
    target_compile_definitions(texgui_visual_regression_test PRIVATE
        TEXGUI_RES_DIR="${MICROTEX_RES_DIR}"
        TEXGUI_VISUAL_BASELINE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/visual_baselines"
    )
    add_test(NAME visual_regression COMMAND texgui_visual_regression_test)
    set_tests_properties(visual_regression PROPERTIES
        ENVIRONMENT "TEXGUI_VISUAL_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/visual-regression"
    )

    # Configure/build/run a tiny external CMake project that consumes texgui via
    # add_subdirectory and compiles as C++17, matching the dependency story.
    add_test(NAME consumer_smoke
        COMMAND ${CMAKE_COMMAND}
                -DTEXGUI_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                -DTEXGUI_CONSUMER_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/consumer-smoke
                -DTEXGUI_CONSUMER_CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DTEXGUI_CONSUMER_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                -DTEXGUI_CONSUMER_GENERATOR=${CMAKE_GENERATOR}
                -DTEXGUI_TEST_CONFIG=$<CONFIG>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/consumer/run_consumer_test.cmake)
endif()

# ---------------------------------------------------------------------------
# Fuzz targets
# ---------------------------------------------------------------------------
if(TEXGUI_BUILD_FUZZERS AND NOT EMSCRIPTEN)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "TEXGUI_BUILD_FUZZERS requires Clang/libFuzzer")
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        message(FATAL_ERROR
            "TEXGUI_BUILD_FUZZERS requires upstream LLVM Clang with libFuzzer. "
            "AppleClang may omit the fuzzer runtime; use Homebrew LLVM or Linux Clang.")
    endif()

    add_executable(texgui_try_render_fuzzer tests/fuzz/try_render_fuzzer.cpp)
    target_link_libraries(texgui_try_render_fuzzer PRIVATE texgui::texgui)
    target_compile_features(texgui_try_render_fuzzer PRIVATE cxx_std_17)
    target_compile_definitions(texgui_try_render_fuzzer PRIVATE TEXGUI_RES_DIR="${MICROTEX_RES_DIR}")
    target_compile_options(texgui_try_render_fuzzer PRIVATE -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer)
    target_link_options(texgui_try_render_fuzzer PRIVATE -fsanitize=fuzzer,address,undefined)
endif()

# ---------------------------------------------------------------------------
# Install / find_package support
# ---------------------------------------------------------------------------
set(_texgui_install_default ${PROJECT_IS_TOP_LEVEL})
if(TEXGUI_RAYLIB_FETCHED)
    set(_texgui_install_default OFF)
endif()
option(TEXGUI_INSTALL "Generate install and find_package(texgui) support" ${_texgui_install_default})

if(TEXGUI_INSTALL AND TEXGUI_RAYLIB_FETCHED)
    message(FATAL_ERROR
        "TEXGUI_INSTALL=ON with a FetchContent-built raylib cannot be exported cleanly. "
        "Install raylib so find_package(raylib) succeeds, or install with -DTEXGUI_BACKEND=headless.")
endif()

if(TEXGUI_INSTALL)
    set(TEXGUI_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/texgui" CACHE STRING "Install directory for texgui CMake package files")
    set(TEXGUI_INSTALL_RESDIR "${CMAKE_INSTALL_DATADIR}/texgui/res")

    set_target_properties(texgui PROPERTIES EXPORT_NAME texgui)

    install(TARGETS texgui microtex_core
        EXPORT texguiTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
    install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(DIRECTORY "${MICROTEX_RES_DIR}/" DESTINATION "${TEXGUI_INSTALL_RESDIR}")
    install(FILES cmake/texguiHelpers.cmake DESTINATION "${TEXGUI_INSTALL_CMAKEDIR}")

    configure_package_config_file(
        cmake/texguiConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/texguiConfig.cmake"
        INSTALL_DESTINATION "${TEXGUI_INSTALL_CMAKEDIR}"
        PATH_VARS TEXGUI_INSTALL_RESDIR
    )
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/texguiConfigVersion.cmake"
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/texguiConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/texguiConfigVersion.cmake"
        DESTINATION "${TEXGUI_INSTALL_CMAKEDIR}"
    )
    install(EXPORT texguiTargets
        NAMESPACE texgui::
        DESTINATION "${TEXGUI_INSTALL_CMAKEDIR}"
    )
endif()
