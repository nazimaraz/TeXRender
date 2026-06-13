cmake_minimum_required(VERSION 3.21)

project(
    latex_renderer
    VERSION 0.1.0
    DESCRIPTION "Render LaTeX formulas via MicroTeX with pluggable C++ rendering backends"
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
set(TEXGUI_BACKEND "raylib" CACHE STRING "Rendering backend to build (raylib, headless, svg, sdl2)")
set_property(CACHE TEXGUI_BACKEND PROPERTY STRINGS raylib headless svg sdl2)
option(TEXGUI_BUILD_EXAMPLES "Build the example applications" ${PROJECT_IS_TOP_LEVEL})
option(TEXGUI_BUILD_TESTS "Build the unit tests" ${PROJECT_IS_TOP_LEVEL})
option(TEXGUI_BUILD_BENCHMARKS "Build the benchmark executables" ${PROJECT_IS_TOP_LEVEL})
option(TEXGUI_BUILD_FUZZERS "Build libFuzzer targets (Clang/libFuzzer)" OFF)
option(TEXGUI_WARNINGS_AS_ERRORS "Treat warnings in texgui first-party targets as errors" OFF)

if(BUILD_SHARED_LIBS)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Default to Release, but only when texgui is the top-level project: a
# dependency must not force a build type into the consuming project's cache.
if(PROJECT_IS_TOP_LEVEL AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
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

function(texgui_target_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        if(TEXGUI_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(TEXGUI_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

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
# pulled in. HeadlessSurface and SvgSurface are dependency-free, so they ship in
# every build (see the library sources below) for offscreen/export rendering.
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
elseif(TEXGUI_BACKEND STREQUAL "svg")
    set(TEXGUI_BACKEND_SOURCES
        source/backend/svg/svg_factory.cpp
    )
    set(TEXGUI_BACKEND_LIBS "")
elseif(TEXGUI_BACKEND STREQUAL "sdl2")
    set(TEXGUI_BACKEND_SOURCES
        source/backend/sdl2/sdl2_surface.cpp
        source/backend/sdl2/sdl2_factory.cpp
    )
    set(TEXGUI_BACKEND_LIBS "${TEXGUI_SDL2_TARGET}")
    set(TEXGUI_BACKEND_EXECUTABLE_LIBS "${TEXGUI_SDL2MAIN_TARGET}")
else()
    message(FATAL_ERROR
        "Unsupported TEXGUI_BACKEND='${TEXGUI_BACKEND}'. Supported: 'raylib', 'headless', 'svg', 'sdl2'.")
endif()

set(TEXGUI_HAS_HEADLESS 1)
set(TEXGUI_HAS_SVG 1)
set(TEXGUI_HAS_RAYLIB 0)
set(TEXGUI_HAS_SDL2 0)
if(TEXGUI_BACKEND STREQUAL "raylib")
    set(TEXGUI_HAS_RAYLIB 1)
elseif(TEXGUI_BACKEND STREQUAL "sdl2")
    set(TEXGUI_HAS_SDL2 1)
endif()
set(TEXGUI_HAS_RAYLIB_CMAKE FALSE)
set(TEXGUI_HAS_SDL2_CMAKE FALSE)
if(TEXGUI_HAS_RAYLIB)
    set(TEXGUI_HAS_RAYLIB_CMAKE TRUE)
endif()
if(TEXGUI_HAS_SDL2)
    set(TEXGUI_HAS_SDL2_CMAKE TRUE)
endif()

set(TEXGUI_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
configure_file(
    cmake/texguiConfig.hpp.in
    "${TEXGUI_GENERATED_INCLUDE_DIR}/texgui/config.hpp"
    @ONLY
)

# ---------------------------------------------------------------------------
# The public library
# ---------------------------------------------------------------------------
add_library(texgui
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
    # The headless canvas is always available (offscreen rendering from any
    # backend); TEXGUI_BACKEND only selects the default surface.
    source/backend/headless/headless_surface.cpp
    ${TEXGUI_BACKEND_SOURCES}
)
add_library(texgui::texgui ALIAS texgui)
texgui_target_warnings(texgui)

target_compile_definitions(texgui PRIVATE TEXGUI_BUILDING_LIBRARY)
get_target_property(_texgui_library_type texgui TYPE)
if(_texgui_library_type STREQUAL "SHARED_LIBRARY" OR _texgui_library_type STREQUAL "MODULE_LIBRARY")
    target_compile_definitions(texgui INTERFACE TEXGUI_SHARED)
endif()
unset(_texgui_library_type)

set_target_properties(texgui PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

target_include_directories(texgui PUBLIC
    $<BUILD_INTERFACE:${TEXGUI_GENERATED_INCLUDE_DIR}>
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
    # user constructs a RaylibSurface or Sdl2Surface), so the backend lib
    # propagates. When a GUI dependency came from FetchContent, it is only a
    # build-tree target; installed packages instead rely on find_dependency()
    # creating that target.
    target_link_libraries(texgui PUBLIC
        "$<BUILD_INTERFACE:${TEXGUI_BACKEND_LIBS}>"
        "$<INSTALL_INTERFACE:${TEXGUI_BACKEND_LIBS}>"
    )
endif()

# texgui's own sources and public headers compile as C++17.
target_compile_features(texgui PRIVATE cxx_std_17)
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
        # HeadlessSurface into an image file, so it builds under any backend with
        # no window or GPU; the raylib examples drive a real window themselves.
        set(TEXGUI_EXAMPLES
            texgui_offscreen examples/offscreen.cpp
            texgui_headless_batch examples/headless_batch.cpp
            texgui_svg_export examples/svg_export.cpp
            texgui_custom_surface examples/custom_surface.cpp
            texgui_formula_label_widget examples/formula_label_widget.cpp
        )
        if(TEXGUI_BACKEND STREQUAL "raylib")
            list(APPEND TEXGUI_EXAMPLES
                texgui_minimal        examples/minimal.cpp
                texgui_demo           examples/demo_main.cpp
                texgui_live           examples/live_edit.cpp
                texgui_raylib_overlay examples/raylib_overlay.cpp
            )
        elseif(TEXGUI_BACKEND STREQUAL "sdl2")
            list(APPEND TEXGUI_EXAMPLES
                texgui_sdl2_minimal examples/sdl2_minimal.cpp
            )
        endif()
    endif()
    while(TEXGUI_EXAMPLES)
        list(POP_FRONT TEXGUI_EXAMPLES example_name example_source)
        add_executable(${example_name} ${example_source})
        target_link_libraries(${example_name} PRIVATE texgui::texgui ${TEXGUI_BACKEND_EXECUTABLE_LIBS})
        texgui_target_warnings(${example_name})
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
    texgui_target_warnings(texgui_render_benchmark)
    target_compile_features(texgui_render_benchmark PRIVATE cxx_std_17)
    texgui_copy_resources(texgui_render_benchmark)
endif()

# ---------------------------------------------------------------------------
# Unit tests (not for the wasm cross-build: they read the MicroTeX res/ tree
# from the host filesystem, which the wasm sandbox cannot see)
# ---------------------------------------------------------------------------
if(TEXGUI_BUILD_TESTS AND NOT EMSCRIPTEN)
    enable_testing()
    find_package(Threads REQUIRED)

    # Pure transform math — no dependencies.
    add_executable(texgui_transform2d_test tests/transform2d_test.cpp)
    target_include_directories(texgui_transform2d_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)
    texgui_target_warnings(texgui_transform2d_test)
    target_compile_features(texgui_transform2d_test PRIVATE cxx_std_17)
    add_test(NAME transform2d COMMAND texgui_transform2d_test)

    # FreeType glyph cache against the bundled TeX fonts — no window/GPU needed.
    add_executable(texgui_glyph_cache_test
        tests/glyph_cache_test.cpp
        source/core/cache_options.cpp
        source/render/glyph_cache.cpp
    )
    target_link_libraries(texgui_glyph_cache_test PRIVATE Freetype::Freetype)
    target_include_directories(texgui_glyph_cache_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/source
    )
    texgui_target_warnings(texgui_glyph_cache_test)
    target_compile_features(texgui_glyph_cache_test PRIVATE cxx_std_17)
    target_compile_definitions(texgui_glyph_cache_test PRIVATE TEXGUI_RES_DIR="${MICROTEX_RES_DIR}")
    add_test(NAME glyph_cache COMMAND texgui_glyph_cache_test)

    # End-to-end render through the public API into an injected headless canvas,
    # inspected pixel-by-pixel. The canvas is pure software, so this runs under
    # any backend with no window, GPU, or display.
    add_executable(texgui_headless_render_test tests/headless_render_test.cpp)
    target_link_libraries(texgui_headless_render_test PRIVATE texgui::texgui)
    texgui_target_warnings(texgui_headless_render_test)
    target_compile_features(texgui_headless_render_test PRIVATE cxx_std_17)
    target_compile_definitions(texgui_headless_render_test PRIVATE TEXGUI_RES_DIR="${MICROTEX_RES_DIR}")
    add_test(NAME headless_render COMMAND texgui_headless_render_test)

    # Structural visual regression over the software surface: stable enough for
    # CI across platforms, but stronger than "some pixels changed".
    add_executable(texgui_visual_regression_test tests/visual_regression_test.cpp)
    target_link_libraries(texgui_visual_regression_test PRIVATE texgui::texgui)
    texgui_target_warnings(texgui_visual_regression_test)
    target_compile_features(texgui_visual_regression_test PRIVATE cxx_std_17)
    target_compile_definitions(texgui_visual_regression_test PRIVATE
        TEXGUI_RES_DIR="${MICROTEX_RES_DIR}"
        TEXGUI_VISUAL_BASELINE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/visual_baselines"
    )
    add_test(NAME visual_regression COMMAND texgui_visual_regression_test)
    set_tests_properties(visual_regression PROPERTIES
        ENVIRONMENT "TEXGUI_VISUAL_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/visual-regression"
    )

    add_executable(texgui_lifetime_stress_test tests/lifetime_stress_test.cpp)
    target_link_libraries(texgui_lifetime_stress_test PRIVATE texgui::texgui Threads::Threads)
    texgui_target_warnings(texgui_lifetime_stress_test)
    target_compile_features(texgui_lifetime_stress_test PRIVATE cxx_std_17)
    target_compile_definitions(texgui_lifetime_stress_test PRIVATE TEXGUI_RES_DIR="${MICROTEX_RES_DIR}")
    add_test(NAME lifetime_stress COMMAND texgui_lifetime_stress_test)

    add_executable(texgui_public_headers_core_cxx17 tests/public_headers_core_cxx17.cpp)
    target_link_libraries(texgui_public_headers_core_cxx17 PRIVATE texgui::texgui)
    texgui_target_warnings(texgui_public_headers_core_cxx17)
    target_compile_features(texgui_public_headers_core_cxx17 PRIVATE cxx_std_17)
    add_test(NAME public_headers_core_cxx17 COMMAND texgui_public_headers_core_cxx17)

    if(TEXGUI_BACKEND STREQUAL "raylib")
        add_executable(texgui_public_headers_raylib_cxx17 tests/public_headers_raylib_cxx17.cpp)
        target_link_libraries(texgui_public_headers_raylib_cxx17 PRIVATE texgui::texgui ${TEXGUI_BACKEND_LIBS})
        texgui_target_warnings(texgui_public_headers_raylib_cxx17)
        target_compile_features(texgui_public_headers_raylib_cxx17 PRIVATE cxx_std_17)
        add_test(NAME public_headers_raylib_cxx17 COMMAND texgui_public_headers_raylib_cxx17)
    elseif(TEXGUI_BACKEND STREQUAL "sdl2")
        add_executable(texgui_public_headers_sdl2_cxx17 tests/public_headers_sdl2_cxx17.cpp)
        target_link_libraries(texgui_public_headers_sdl2_cxx17 PRIVATE texgui::texgui ${TEXGUI_BACKEND_EXECUTABLE_LIBS})
        texgui_target_warnings(texgui_public_headers_sdl2_cxx17)
        target_compile_features(texgui_public_headers_sdl2_cxx17 PRIVATE cxx_std_17)
        add_test(NAME public_headers_sdl2_cxx17 COMMAND texgui_public_headers_sdl2_cxx17)

        add_executable(texgui_sdl2_surface_test tests/sdl2_surface_test.cpp)
        target_link_libraries(texgui_sdl2_surface_test PRIVATE texgui::texgui ${TEXGUI_BACKEND_EXECUTABLE_LIBS})
        texgui_target_warnings(texgui_sdl2_surface_test)
        target_compile_features(texgui_sdl2_surface_test PRIVATE cxx_std_17)
        target_compile_definitions(texgui_sdl2_surface_test PRIVATE TEXGUI_RES_DIR="${MICROTEX_RES_DIR}")
        add_test(NAME sdl2_surface COMMAND texgui_sdl2_surface_test)
        set_tests_properties(sdl2_surface PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy")
    endif()

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
    texgui_target_warnings(texgui_try_render_fuzzer)
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
if(TEXGUI_SDL2_FETCHED)
    set(_texgui_install_default OFF)
endif()
option(TEXGUI_INSTALL "Generate install and find_package(texgui) support" ${_texgui_install_default})

if(TEXGUI_INSTALL AND TEXGUI_RAYLIB_FETCHED)
    message(FATAL_ERROR
        "TEXGUI_INSTALL=ON with a FetchContent-built raylib cannot be exported cleanly. "
        "Install raylib so find_package(raylib) succeeds, or install with -DTEXGUI_BACKEND=headless "
        "or -DTEXGUI_BACKEND=svg.")
endif()

if(TEXGUI_INSTALL AND TEXGUI_SDL2_FETCHED)
    message(FATAL_ERROR
        "TEXGUI_INSTALL=ON with a FetchContent-built SDL2 cannot be exported cleanly. "
        "Install SDL2 so find_package(SDL2) succeeds, or install with -DTEXGUI_BACKEND=headless "
        "or -DTEXGUI_BACKEND=svg.")
endif()

if(TEXGUI_INSTALL)
    set(TEXGUI_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/texgui" CACHE STRING "Install directory for texgui CMake package files")
    set(TEXGUI_INSTALL_PKGCONFIGDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig" CACHE STRING "Install directory for texgui pkg-config metadata")
    set(TEXGUI_INSTALL_RESDIR "${CMAKE_INSTALL_DATADIR}/texgui/res")
    set(TEXGUI_PUBLIC_HEADERS
        include/texgui/cache_options.hpp
        "${TEXGUI_GENERATED_INCLUDE_DIR}/texgui/config.hpp"
        include/texgui/draw_surface.hpp
        include/texgui/export.hpp
        include/texgui/formula_layout.hpp
        include/texgui/glyph_bitmap.hpp
        include/texgui/headless_surface.hpp
        include/texgui/latex_document.hpp
        include/texgui/render_handle.hpp
        include/texgui/svg_surface.hpp
        include/texgui/texgui.hpp
        include/texgui/version.hpp
    )
    if(TEXGUI_BACKEND STREQUAL "raylib")
        list(APPEND TEXGUI_PUBLIC_HEADERS include/texgui/raylib_surface.hpp)
    elseif(TEXGUI_BACKEND STREQUAL "sdl2")
        list(APPEND TEXGUI_PUBLIC_HEADERS include/texgui/sdl2_surface.hpp)
    endif()

    set_target_properties(texgui PROPERTIES EXPORT_NAME texgui)

    install(TARGETS texgui microtex_core
        EXPORT texguiTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
    install(FILES ${TEXGUI_PUBLIC_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/texgui")
    install(DIRECTORY "${MICROTEX_RES_DIR}/" DESTINATION "${TEXGUI_INSTALL_RESDIR}")
    install(FILES cmake/texguiHelpers.cmake DESTINATION "${TEXGUI_INSTALL_CMAKEDIR}")

    configure_package_config_file(
        cmake/texguiConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/texguiConfig.cmake"
        INSTALL_DESTINATION "${TEXGUI_INSTALL_CMAKEDIR}"
        PATH_VARS TEXGUI_INSTALL_RESDIR
    )
    set(TEXGUI_PKG_CONFIG_REQUIRES_PRIVATE "freetype2 tinyxml2")
    if(TEXGUI_BACKEND STREQUAL "raylib")
        string(APPEND TEXGUI_PKG_CONFIG_REQUIRES_PRIVATE " raylib")
    elseif(TEXGUI_BACKEND STREQUAL "sdl2")
        string(APPEND TEXGUI_PKG_CONFIG_REQUIRES_PRIVATE " sdl2")
    endif()
    configure_file(
        cmake/texgui.pc.in
        "${CMAKE_CURRENT_BINARY_DIR}/texgui.pc"
        @ONLY
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
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/texgui.pc"
        DESTINATION "${TEXGUI_INSTALL_PKGCONFIGDIR}"
    )
endif()

# ---------------------------------------------------------------------------
# CPack packages
# ---------------------------------------------------------------------------
if(PROJECT_IS_TOP_LEVEL AND NOT EMSCRIPTEN)
    set(CPACK_PACKAGE_NAME "latex-renderer")
    set(CPACK_PACKAGE_VENDOR "Nazim Can")
    set(CPACK_PACKAGE_CONTACT "https://github.com/nazimaraz/latex-renderer")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_FILE_NAME "latex-renderer-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_GENERATOR "TGZ;ZIP")

    set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
    set(CPACK_SOURCE_PACKAGE_FILE_NAME "latex-renderer-${PROJECT_VERSION}-source")
    set(CPACK_SOURCE_IGNORE_FILES
        "/\\\\.git/"
        "/\\\\.github/workflows/.*~$"
        "/build[^/]*/"
        "/out/"
        "/build-api-docs/"
        "/\\\\.cache/"
        "/\\\\.DS_Store$"
        "/__pycache__/"
        "\\\\.pyc$"
    )

    include(CPack)
endif()
