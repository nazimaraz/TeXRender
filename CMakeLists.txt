cmake_minimum_required(VERSION 3.21)

# Top-level macOS builds should target the host by default so locally installed
# dependencies, especially Homebrew dylibs, do not link against a newer macOS
# than this project requests. Package managers and consumers can still set
# CMAKE_OSX_DEPLOYMENT_TARGET explicitly.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR
   AND CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin"
   AND NOT CMAKE_OSX_DEPLOYMENT_TARGET
   AND "$ENV{MACOSX_DEPLOYMENT_TARGET}" STREQUAL "")
    execute_process(
        COMMAND sw_vers -productVersion
        OUTPUT_VARIABLE _texrender_host_macos_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    string(REGEX MATCH "^[0-9]+\\.[0-9]+" _texrender_host_macos_target "${_texrender_host_macos_version}")
    if(_texrender_host_macos_target)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "${_texrender_host_macos_target}" CACHE STRING
            "Minimum macOS version to target" FORCE)
    endif()
    unset(_texrender_host_macos_target)
    unset(_texrender_host_macos_version)
endif()

project(
    texrender
    VERSION 0.1.0
    DESCRIPTION "Render LaTeX formulas via MicroTeX with pluggable C++ rendering backends"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# The public version header is hand-written (so it is browsable in the repo);
# fail the configure if it drifts from project(VERSION) above.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/texrender/version.hpp" _texrender_version_header)
if(NOT _texrender_version_header MATCHES "#define TEXRENDER_VERSION_MAJOR ${PROJECT_VERSION_MAJOR}\n"
   OR NOT _texrender_version_header MATCHES "#define TEXRENDER_VERSION_MINOR ${PROJECT_VERSION_MINOR}\n"
   OR NOT _texrender_version_header MATCHES "#define TEXRENDER_VERSION_PATCH ${PROJECT_VERSION_PATCH}\n")
    message(FATAL_ERROR
        "include/texrender/version.hpp is out of sync with project(VERSION ${PROJECT_VERSION}).")
endif()
unset(_texrender_version_header)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
set(TEXRENDER_BACKEND "headless" CACHE STRING "Rendering backend to build (headless, svg, raylib, sdl2)")
set_property(CACHE TEXRENDER_BACKEND PROPERTY STRINGS raylib headless svg sdl2)
option(TEXRENDER_BUILD_EXAMPLES "Build the example applications" ${PROJECT_IS_TOP_LEVEL})
option(TEXRENDER_BUILD_TESTS "Build the unit tests" ${PROJECT_IS_TOP_LEVEL})
option(TEXRENDER_BUILD_BENCHMARKS "Build the benchmark executables" ${PROJECT_IS_TOP_LEVEL})
option(TEXRENDER_BUILD_FUZZERS "Build libFuzzer targets (Clang/libFuzzer)" OFF)
option(TEXRENDER_WARNINGS_AS_ERRORS "Treat warnings in texrender first-party targets as errors" OFF)

if(BUILD_SHARED_LIBS)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Default to Release, but only when texrender is the top-level project: a
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

function(texrender_target_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        if(TEXRENDER_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(TEXRENDER_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

# ---------------------------------------------------------------------------
# Dependencies: MicroTeX core, FreeType, tinyxml2, and the selected backend.
# ---------------------------------------------------------------------------
include(cmake/Dependencies.cmake)

set(TEXRENDER_RESOURCE_DIR "${MICROTEX_RES_DIR}" CACHE INTERNAL "texrender MicroTeX resource directory")
include(cmake/texrenderHelpers.cmake)

# ---------------------------------------------------------------------------
# Backend selection (Open/Closed: add a new backend => add a branch + sources)
# ---------------------------------------------------------------------------
# TEXRENDER_BACKEND picks which surface make_surface() creates by default (the
# *_factory.cpp that gets compiled) and which backend-specific sources/libs are
# pulled in. HeadlessSurface and SvgSurface are dependency-free, so they ship in
# every build (see the library sources below) for offscreen/export rendering.
if(TEXRENDER_BACKEND STREQUAL "raylib")
    set(TEXRENDER_BACKEND_SOURCES
        source/backend/raylib/raylib_surface.cpp
        source/backend/raylib/raylib_factory.cpp
    )
    set(TEXRENDER_BACKEND_LIBS raylib)
elseif(TEXRENDER_BACKEND STREQUAL "headless")
    set(TEXRENDER_BACKEND_SOURCES
        source/backend/headless/headless_factory.cpp
    )
    set(TEXRENDER_BACKEND_LIBS "")
elseif(TEXRENDER_BACKEND STREQUAL "svg")
    set(TEXRENDER_BACKEND_SOURCES
        source/backend/svg/svg_factory.cpp
    )
    set(TEXRENDER_BACKEND_LIBS "")
elseif(TEXRENDER_BACKEND STREQUAL "sdl2")
    set(TEXRENDER_BACKEND_SOURCES
        source/backend/sdl2/sdl2_surface.cpp
        source/backend/sdl2/sdl2_factory.cpp
    )
    set(TEXRENDER_BACKEND_LIBS "${TEXRENDER_SDL2_TARGET}")
    set(TEXRENDER_BACKEND_EXECUTABLE_LIBS "${TEXRENDER_SDL2MAIN_TARGET}")
else()
    message(FATAL_ERROR
        "Unsupported TEXRENDER_BACKEND='${TEXRENDER_BACKEND}'. Supported: 'raylib', 'headless', 'svg', 'sdl2'.")
endif()

set(TEXRENDER_HAS_HEADLESS 1)
set(TEXRENDER_HAS_SVG 1)
set(TEXRENDER_HAS_RAYLIB 0)
set(TEXRENDER_HAS_SDL2 0)
if(TEXRENDER_BACKEND STREQUAL "raylib")
    set(TEXRENDER_HAS_RAYLIB 1)
elseif(TEXRENDER_BACKEND STREQUAL "sdl2")
    set(TEXRENDER_HAS_SDL2 1)
endif()
set(TEXRENDER_HAS_RAYLIB_CMAKE FALSE)
set(TEXRENDER_HAS_SDL2_CMAKE FALSE)
if(TEXRENDER_HAS_RAYLIB)
    set(TEXRENDER_HAS_RAYLIB_CMAKE TRUE)
endif()
if(TEXRENDER_HAS_SDL2)
    set(TEXRENDER_HAS_SDL2_CMAKE TRUE)
endif()

set(TEXRENDER_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
configure_file(
    cmake/texrenderConfig.hpp.in
    "${TEXRENDER_GENERATED_INCLUDE_DIR}/texrender/config.hpp"
    @ONLY
)

# ---------------------------------------------------------------------------
# The public library
# ---------------------------------------------------------------------------
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
    # The headless canvas is always available (offscreen rendering from any
    # backend); TEXRENDER_BACKEND only selects the default surface.
    source/backend/headless/headless_surface.cpp
    ${TEXRENDER_BACKEND_SOURCES}
)
add_library(texrender::texrender ALIAS texrender)
texrender_target_warnings(texrender)

target_compile_definitions(texrender PRIVATE TEXRENDER_BUILDING_LIBRARY)
get_target_property(_texrender_library_type texrender TYPE)
if(_texrender_library_type STREQUAL "SHARED_LIBRARY" OR _texrender_library_type STREQUAL "MODULE_LIBRARY")
    target_compile_definitions(texrender INTERFACE TEXRENDER_SHARED)
endif()
unset(_texrender_library_type)

set_target_properties(texrender PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

target_include_directories(texrender PUBLIC
    $<BUILD_INTERFACE:${TEXRENDER_GENERATED_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(texrender PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)
target_include_directories(texrender SYSTEM PRIVATE
    ${MICROTEX_SRC}
)

target_link_libraries(texrender
    PRIVATE microtex_core Freetype::Freetype
)
if(TEXRENDER_BACKEND_LIBS)
    # The selected backend's headers are part of our public surface (e.g. the
    # user constructs a RaylibSurface or Sdl2Surface), so the backend lib
    # propagates. When a GUI dependency came from FetchContent, it is only a
    # build-tree target; installed packages instead rely on find_dependency()
    # creating that target.
    target_link_libraries(texrender PUBLIC
        "$<BUILD_INTERFACE:${TEXRENDER_BACKEND_LIBS}>"
        "$<INSTALL_INTERFACE:${TEXRENDER_BACKEND_LIBS}>"
    )
endif()

# texrender's own sources and public headers compile as C++17.
target_compile_features(texrender PRIVATE cxx_std_17)
target_compile_features(texrender INTERFACE cxx_std_17)

set(TEXRENDER_MACOS_DEPLOYMENT_LINK_FLAG "")
if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    set(TEXRENDER_MACOS_DEPLOYMENT_LINK_FLAG "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    target_link_options(texrender INTERFACE "${TEXRENDER_MACOS_DEPLOYMENT_LINK_FLAG}")
endif()

# Non-Apple Clang on macOS (e.g. Homebrew's llvm / llvm@NN) compiles against its
# own libc++ headers but does not add that libc++ to the link line. Linking
# MicroTeX then fails with undefined std::__1::...[abi:...] symbols and missing
# stringstream / bad_function_call vtables. Apple Clang ships a matched libc++
# and is unaffected (its compiler id is "AppleClang"). Derive the toolchain's
# own libc++ directory from the compiler location and link it. BUILD_INTERFACE
# keeps this machine-specific path out of the installed/exported package, and
# PRIVATE covers texrender's own link in BUILD_SHARED_LIBS builds.
if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    get_filename_component(_texrender_clang_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(_texrender_toolchain_root "${_texrender_clang_bin}" DIRECTORY)
    set(_texrender_libcxx_dir "")
    foreach(_candidate "${_texrender_toolchain_root}/lib/c++" "${_texrender_toolchain_root}/lib")
        if(EXISTS "${_candidate}/libc++.dylib" OR EXISTS "${_candidate}/libc++.1.dylib")
            set(_texrender_libcxx_dir "${_candidate}")
            break()
        endif()
    endforeach()
    if(_texrender_libcxx_dir)
        message(STATUS "texrender: linking non-Apple Clang libc++ from ${_texrender_libcxx_dir}")
        target_link_options(texrender PRIVATE "-L${_texrender_libcxx_dir}" "-Wl,-rpath,${_texrender_libcxx_dir}")
        target_link_options(texrender INTERFACE
            "$<BUILD_INTERFACE:-L${_texrender_libcxx_dir}>"
            "$<BUILD_INTERFACE:-Wl,-rpath,${_texrender_libcxx_dir}>")
    endif()
    unset(_texrender_clang_bin)
    unset(_texrender_toolchain_root)
    unset(_texrender_libcxx_dir)
endif()

# ---------------------------------------------------------------------------
# Example applications
# ---------------------------------------------------------------------------
if(TEXRENDER_BUILD_EXAMPLES)
    if(EMSCRIPTEN AND TEXRENDER_BACKEND STREQUAL "raylib")
        # Browser build: only the web-adapted live editor (a blocking
        # while-loop example would freeze the tab). It is emitted into
        # <build>/site/ as index.html + .js/.wasm/.data — a directory ready
        # for any static host (GitHub Pages).
        set(TEXRENDER_EXAMPLES
            texrender_live examples/live_edit.cpp
        )
    else()
        # name -> source file. texrender_offscreen renders through an injected
        # HeadlessSurface into an image file, so it builds under any backend with
        # no window or GPU; the raylib examples drive a real window themselves.
        set(TEXRENDER_EXAMPLES
            texrender_offscreen examples/offscreen.cpp
            texrender_headless_batch examples/headless_batch.cpp
            texrender_untrusted_worker examples/untrusted_worker.cpp
            texrender_svg_export examples/svg_export.cpp
            texrender_custom_surface examples/custom_surface.cpp
            texrender_formula_label_widget examples/formula_label_widget.cpp
        )
        if(TEXRENDER_BACKEND STREQUAL "raylib")
            list(APPEND TEXRENDER_EXAMPLES
                texrender_minimal        examples/minimal.cpp
                texrender_demo           examples/demo_main.cpp
                texrender_realtime_values examples/realtime_values.cpp
                texrender_live           examples/live_edit.cpp
                texrender_raylib_overlay examples/raylib_overlay.cpp
            )
        elseif(TEXRENDER_BACKEND STREQUAL "sdl2")
            list(APPEND TEXRENDER_EXAMPLES
                texrender_sdl2_minimal examples/sdl2_minimal.cpp
            )
        endif()
    endif()
    while(TEXRENDER_EXAMPLES)
        list(POP_FRONT TEXRENDER_EXAMPLES example_name example_source)
        add_executable(${example_name} ${example_source})
        target_link_libraries(${example_name} PRIVATE texrender::texrender ${TEXRENDER_BACKEND_EXECUTABLE_LIBS})
        texrender_target_warnings(${example_name})
        # Examples are consumer-shaped code: pinning them to C++17 makes every
        # build prove the public headers require nothing newer than that.
        set_target_properties(${example_name} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
        # MicroTeX needs its resource directory (fonts + metrics) at runtime.
        # On the web that directory is packed into the .data file below and
        # mounted at /res in the virtual filesystem.
        if(EMSCRIPTEN)
            target_compile_definitions(${example_name} PRIVATE TEXRENDER_RES_DIR="/res")
        else()
            texrender_copy_resources(${example_name})
        endif()
    endwhile()

    if(TARGET texrender_live AND EMSCRIPTEN)
        set_target_properties(texrender_live PROPERTIES
            OUTPUT_NAME index
            SUFFIX ".html"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/site"
        )
        target_link_options(texrender_live PRIVATE
            "-sUSE_GLFW=3"                # raylib's web backend speaks GLFW
            "-sALLOW_MEMORY_GROWTH=1"
            "-sSTACK_SIZE=2097152"        # MicroTeX's parser recurses; default 64K is tight
            "SHELL:--preload-file ${MICROTEX_RES_DIR}@/res"
            "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/examples/web/shell.html"
        )
        set_property(TARGET texrender_live APPEND PROPERTY
            LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/examples/web/shell.html")
    endif()
endif()

# ---------------------------------------------------------------------------
# Benchmarks
# ---------------------------------------------------------------------------
if(TEXRENDER_BUILD_BENCHMARKS AND NOT EMSCRIPTEN)
    add_executable(texrender_render_benchmark benchmarks/render_benchmark.cpp)
    target_link_libraries(texrender_render_benchmark PRIVATE texrender::texrender)
    texrender_target_warnings(texrender_render_benchmark)
    target_compile_features(texrender_render_benchmark PRIVATE cxx_std_17)
    texrender_copy_resources(texrender_render_benchmark)
endif()

# ---------------------------------------------------------------------------
# Unit tests (not for the wasm cross-build: they read the MicroTeX res/ tree
# from the host filesystem, which the wasm sandbox cannot see)
# ---------------------------------------------------------------------------
if(TEXRENDER_BUILD_TESTS AND NOT EMSCRIPTEN)
    enable_testing()
    find_package(Threads REQUIRED)

    # Pure transform math — no dependencies.
    add_executable(texrender_transform2d_test tests/transform2d_test.cpp)
    target_include_directories(texrender_transform2d_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)
    texrender_target_warnings(texrender_transform2d_test)
    target_compile_features(texrender_transform2d_test PRIVATE cxx_std_17)
    add_test(NAME transform2d COMMAND texrender_transform2d_test)

    # FreeType glyph cache against the bundled TeX fonts — no window/GPU needed.
    add_executable(texrender_glyph_cache_test
        tests/glyph_cache_test.cpp
        source/core/cache_options.cpp
        source/render/glyph_cache.cpp
    )
    target_link_libraries(texrender_glyph_cache_test PRIVATE Freetype::Freetype)
    target_include_directories(texrender_glyph_cache_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/source
    )
    texrender_target_warnings(texrender_glyph_cache_test)
    target_compile_features(texrender_glyph_cache_test PRIVATE cxx_std_17)
    target_compile_definitions(texrender_glyph_cache_test PRIVATE TEXRENDER_RES_DIR="${MICROTEX_RES_DIR}")
    add_test(NAME glyph_cache COMMAND texrender_glyph_cache_test)

    # End-to-end render through the public API into an injected headless canvas,
    # inspected pixel-by-pixel. The canvas is pure software, so this runs under
    # any backend with no window, GPU, or display.
    add_executable(texrender_headless_render_test tests/headless_render_test.cpp)
    target_link_libraries(texrender_headless_render_test PRIVATE texrender::texrender)
    texrender_target_warnings(texrender_headless_render_test)
    target_compile_features(texrender_headless_render_test PRIVATE cxx_std_17)
    target_compile_definitions(texrender_headless_render_test PRIVATE TEXRENDER_RES_DIR="${MICROTEX_RES_DIR}")
    add_test(NAME headless_render COMMAND texrender_headless_render_test)

    # Structural visual regression over the software surface: stable enough for
    # CI across platforms, but stronger than "some pixels changed".
    add_executable(texrender_visual_regression_test tests/visual_regression_test.cpp)
    target_link_libraries(texrender_visual_regression_test PRIVATE texrender::texrender)
    texrender_target_warnings(texrender_visual_regression_test)
    target_compile_features(texrender_visual_regression_test PRIVATE cxx_std_17)
    target_compile_definitions(texrender_visual_regression_test PRIVATE
        TEXRENDER_RES_DIR="${MICROTEX_RES_DIR}"
        TEXRENDER_VISUAL_BASELINE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/visual_baselines"
    )
    add_test(NAME visual_regression COMMAND texrender_visual_regression_test)
    set_tests_properties(visual_regression PROPERTIES
        ENVIRONMENT "TEXRENDER_VISUAL_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/visual-regression"
    )

    add_executable(texrender_lifetime_stress_test tests/lifetime_stress_test.cpp)
    target_link_libraries(texrender_lifetime_stress_test PRIVATE texrender::texrender Threads::Threads)
    texrender_target_warnings(texrender_lifetime_stress_test)
    target_compile_features(texrender_lifetime_stress_test PRIVATE cxx_std_17)
    target_compile_definitions(texrender_lifetime_stress_test PRIVATE TEXRENDER_RES_DIR="${MICROTEX_RES_DIR}")
    add_test(NAME lifetime_stress COMMAND texrender_lifetime_stress_test)

    add_executable(texrender_public_headers_core_cxx17 tests/public_headers_core_cxx17.cpp)
    target_link_libraries(texrender_public_headers_core_cxx17 PRIVATE texrender::texrender)
    texrender_target_warnings(texrender_public_headers_core_cxx17)
    target_compile_features(texrender_public_headers_core_cxx17 PRIVATE cxx_std_17)
    add_test(NAME public_headers_core_cxx17 COMMAND texrender_public_headers_core_cxx17)

    if(TEXRENDER_BACKEND STREQUAL "raylib")
        add_executable(texrender_public_headers_raylib_cxx17 tests/public_headers_raylib_cxx17.cpp)
        target_link_libraries(texrender_public_headers_raylib_cxx17 PRIVATE texrender::texrender ${TEXRENDER_BACKEND_LIBS})
        texrender_target_warnings(texrender_public_headers_raylib_cxx17)
        target_compile_features(texrender_public_headers_raylib_cxx17 PRIVATE cxx_std_17)
        add_test(NAME public_headers_raylib_cxx17 COMMAND texrender_public_headers_raylib_cxx17)
    elseif(TEXRENDER_BACKEND STREQUAL "sdl2")
        add_executable(texrender_public_headers_sdl2_cxx17 tests/public_headers_sdl2_cxx17.cpp)
        target_link_libraries(texrender_public_headers_sdl2_cxx17 PRIVATE texrender::texrender ${TEXRENDER_BACKEND_EXECUTABLE_LIBS})
        texrender_target_warnings(texrender_public_headers_sdl2_cxx17)
        target_compile_features(texrender_public_headers_sdl2_cxx17 PRIVATE cxx_std_17)
        add_test(NAME public_headers_sdl2_cxx17 COMMAND texrender_public_headers_sdl2_cxx17)

        add_executable(texrender_sdl2_surface_test tests/sdl2_surface_test.cpp)
        target_link_libraries(texrender_sdl2_surface_test PRIVATE texrender::texrender ${TEXRENDER_BACKEND_EXECUTABLE_LIBS})
        texrender_target_warnings(texrender_sdl2_surface_test)
        target_compile_features(texrender_sdl2_surface_test PRIVATE cxx_std_17)
        target_compile_definitions(texrender_sdl2_surface_test PRIVATE TEXRENDER_RES_DIR="${MICROTEX_RES_DIR}")
        add_test(NAME sdl2_surface COMMAND texrender_sdl2_surface_test)
        set_tests_properties(sdl2_surface PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy")
    endif()

    # Configure/build/run a tiny external CMake project that consumes texrender via
    # add_subdirectory and compiles as C++17, matching the dependency story.
    add_test(NAME consumer_smoke
        COMMAND ${CMAKE_COMMAND}
                -DTEXRENDER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                -DTEXRENDER_CONSUMER_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/consumer-smoke
                -DTEXRENDER_CONSUMER_CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DTEXRENDER_CONSUMER_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                -DTEXRENDER_CONSUMER_GENERATOR=${CMAKE_GENERATOR}
                -DTEXRENDER_TEST_CONFIG=$<CONFIG>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/consumer/run_consumer_test.cmake)
endif()

# ---------------------------------------------------------------------------
# Fuzz targets
# ---------------------------------------------------------------------------
if(TEXRENDER_BUILD_FUZZERS AND NOT EMSCRIPTEN)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "TEXRENDER_BUILD_FUZZERS requires Clang/libFuzzer")
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        message(FATAL_ERROR
            "TEXRENDER_BUILD_FUZZERS requires upstream LLVM Clang with libFuzzer. "
            "AppleClang may omit the fuzzer runtime; use Homebrew LLVM or Linux Clang.")
    endif()

    add_executable(texrender_try_render_fuzzer tests/fuzz/try_render_fuzzer.cpp)
    target_link_libraries(texrender_try_render_fuzzer PRIVATE texrender::texrender)
    texrender_target_warnings(texrender_try_render_fuzzer)
    target_compile_features(texrender_try_render_fuzzer PRIVATE cxx_std_17)
    target_compile_definitions(texrender_try_render_fuzzer PRIVATE TEXRENDER_RES_DIR="${MICROTEX_RES_DIR}")
    target_compile_options(texrender_try_render_fuzzer PRIVATE -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer)
    target_link_options(texrender_try_render_fuzzer PRIVATE -fsanitize=fuzzer,address,undefined)
endif()

# ---------------------------------------------------------------------------
# Install / find_package support
# ---------------------------------------------------------------------------
set(_texrender_install_default ${PROJECT_IS_TOP_LEVEL})
if(TEXRENDER_TINYXML2_FETCHED)
    set(_texrender_install_default OFF)
endif()
if(TEXRENDER_FREETYPE_FETCHED)
    set(_texrender_install_default OFF)
endif()
if(TEXRENDER_RAYLIB_FETCHED)
    set(_texrender_install_default OFF)
endif()
if(TEXRENDER_SDL2_FETCHED)
    set(_texrender_install_default OFF)
endif()
option(TEXRENDER_INSTALL "Generate install and find_package(texrender) support" ${_texrender_install_default})

if(TEXRENDER_INSTALL AND TEXRENDER_TINYXML2_FETCHED)
    message(FATAL_ERROR
        "TEXRENDER_INSTALL=ON with a FetchContent-built tinyxml2 cannot be exported cleanly. "
        "Install tinyxml2 so find_package(tinyxml2 CONFIG) succeeds, or consume texrender with "
        "FetchContent/add_subdirectory instead of installing it.")
endif()

if(TEXRENDER_INSTALL AND TEXRENDER_FREETYPE_FETCHED)
    message(FATAL_ERROR
        "TEXRENDER_INSTALL=ON with a FetchContent-built FreeType cannot be exported cleanly. "
        "Install FreeType so find_package(Freetype) succeeds, or consume texrender with "
        "FetchContent/add_subdirectory instead of installing it.")
endif()

if(TEXRENDER_INSTALL AND TEXRENDER_RAYLIB_FETCHED)
    message(FATAL_ERROR
        "TEXRENDER_INSTALL=ON with a FetchContent-built raylib cannot be exported cleanly. "
        "Install raylib so find_package(raylib) succeeds, or install with -DTEXRENDER_BACKEND=headless "
        "or -DTEXRENDER_BACKEND=svg.")
endif()

if(TEXRENDER_INSTALL AND TEXRENDER_SDL2_FETCHED)
    message(FATAL_ERROR
        "TEXRENDER_INSTALL=ON with a FetchContent-built SDL2 cannot be exported cleanly. "
        "Install SDL2 so find_package(SDL2) succeeds, or install with -DTEXRENDER_BACKEND=headless "
        "or -DTEXRENDER_BACKEND=svg.")
endif()

if(TEXRENDER_INSTALL)
    set(TEXRENDER_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/texrender" CACHE STRING "Install directory for texrender CMake package files")
    set(TEXRENDER_INSTALL_PKGCONFIGDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig" CACHE STRING "Install directory for texrender pkg-config metadata")
    set(TEXRENDER_INSTALL_RESDIR "${CMAKE_INSTALL_DATADIR}/texrender/res")
    set(TEXRENDER_PUBLIC_HEADERS
        include/texrender/cache_options.hpp
        "${TEXRENDER_GENERATED_INCLUDE_DIR}/texrender/config.hpp"
        include/texrender/draw_surface.hpp
        include/texrender/export.hpp
        include/texrender/formula_layout.hpp
        include/texrender/glyph_bitmap.hpp
        include/texrender/headless_surface.hpp
        include/texrender/latex_document.hpp
        include/texrender/render_handle.hpp
        include/texrender/svg_surface.hpp
        include/texrender/texrender.hpp
        include/texrender/version.hpp
    )
    if(TEXRENDER_BACKEND STREQUAL "raylib")
        list(APPEND TEXRENDER_PUBLIC_HEADERS include/texrender/raylib_surface.hpp)
    elseif(TEXRENDER_BACKEND STREQUAL "sdl2")
        list(APPEND TEXRENDER_PUBLIC_HEADERS include/texrender/sdl2_surface.hpp)
    endif()

    set_target_properties(texrender PROPERTIES EXPORT_NAME texrender)

    install(TARGETS texrender microtex_core
        EXPORT texrenderTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
    install(FILES ${TEXRENDER_PUBLIC_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/texrender")
    install(DIRECTORY "${MICROTEX_RES_DIR}/" DESTINATION "${TEXRENDER_INSTALL_RESDIR}")
    install(FILES
        LICENSE
        THIRD_PARTY.md
        docs/dependency-license-audit.md
        docs/upstream-microtex-patch.md
        DESTINATION "${CMAKE_INSTALL_DOCDIR}"
    )
    install(FILES cmake/texrenderHelpers.cmake DESTINATION "${TEXRENDER_INSTALL_CMAKEDIR}")

    configure_package_config_file(
        cmake/texrenderConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/texrenderConfig.cmake"
        INSTALL_DESTINATION "${TEXRENDER_INSTALL_CMAKEDIR}"
        PATH_VARS TEXRENDER_INSTALL_RESDIR
    )
    set(TEXRENDER_PKG_CONFIG_REQUIRES_PRIVATE "freetype2 tinyxml2")
    if(TEXRENDER_BACKEND STREQUAL "raylib")
        string(APPEND TEXRENDER_PKG_CONFIG_REQUIRES_PRIVATE " raylib")
    elseif(TEXRENDER_BACKEND STREQUAL "sdl2")
        string(APPEND TEXRENDER_PKG_CONFIG_REQUIRES_PRIVATE " sdl2")
    endif()
    set(TEXRENDER_PKG_CONFIG_LINK_FLAGS "")
    if(TEXRENDER_MACOS_DEPLOYMENT_LINK_FLAG)
        set(TEXRENDER_PKG_CONFIG_LINK_FLAGS " ${TEXRENDER_MACOS_DEPLOYMENT_LINK_FLAG}")
    endif()
    configure_file(
        cmake/texrender.pc.in
        "${CMAKE_CURRENT_BINARY_DIR}/texrender.pc"
        @ONLY
    )
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/texrenderConfigVersion.cmake"
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/texrenderConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/texrenderConfigVersion.cmake"
        DESTINATION "${TEXRENDER_INSTALL_CMAKEDIR}"
    )
    install(EXPORT texrenderTargets
        NAMESPACE texrender::
        DESTINATION "${TEXRENDER_INSTALL_CMAKEDIR}"
    )
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/texrender.pc"
        DESTINATION "${TEXRENDER_INSTALL_PKGCONFIGDIR}"
    )
endif()

# ---------------------------------------------------------------------------
# CPack packages
# ---------------------------------------------------------------------------
if(PROJECT_IS_TOP_LEVEL AND NOT EMSCRIPTEN)
    set(CPACK_PACKAGE_NAME "TeXRender")
    set(CPACK_PACKAGE_VENDOR "Nazim Can")
    set(CPACK_PACKAGE_CONTACT "https://github.com/nazimaraz/TeXRender")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_FILE_NAME "TeXRender-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_GENERATOR "TGZ;ZIP")

    set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
    set(CPACK_SOURCE_PACKAGE_FILE_NAME "TeXRender-${PROJECT_VERSION}-source")
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
