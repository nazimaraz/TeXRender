cmake_minimum_required(VERSION 3.21)
project(TeXRender VERSION 0.1.0 DESCRIPTION "Render LaTeX formulas via MicroTeX with pluggable C++ rendering backends" LANGUAGES CXX)

set(TEXRENDER_BACKEND "headless" CACHE STRING "Rendering backend to build (headless, svg, raylib, sdl2)")
set_property(CACHE TEXRENDER_BACKEND PROPERTY STRINGS raylib headless svg sdl2)
option(TEXRENDER_BUILD_EXAMPLES "Build the example applications" ${PROJECT_IS_TOP_LEVEL})
option(TEXRENDER_BUILD_TESTS "Build the unit tests" ${PROJECT_IS_TOP_LEVEL})
option(TEXRENDER_BUILD_BENCHMARKS "Build the benchmark executables" ${PROJECT_IS_TOP_LEVEL})
option(TEXRENDER_BUILD_FUZZERS "Build libFuzzer targets (Clang/libFuzzer)" OFF)
option(TEXRENDER_USE_SUBMODULE_DEPENDENCIES "Use third_party dependency submodules before network FetchContent fallbacks" ON)
option(TEXRENDER_FETCH_DEPENDENCIES "Allow network FetchContent fallback when required dependency packages and submodules are missing" ON)
option(TEXRENDER_FETCH_GUI_DEPENDENCIES "Allow network FetchContent fallback when selected GUI dependency packages and submodules are missing" OFF)
option(TEXRENDER_WARNINGS_AS_ERRORS "Treat warnings in texrender first-party targets as errors" OFF)
if (BUILD_SHARED_LIBS)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif ()

if (PROJECT_IS_TOP_LEVEL AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif ()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if (EMSCRIPTEN)
    add_compile_options(-fwasm-exceptions)
    add_link_options(-fwasm-exceptions)
endif ()

function(texrender_target_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4)
        if (TEXRENDER_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif ()
    else ()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if (TEXRENDER_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif ()
    endif ()
endfunction()

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

set(TEXRENDER_HAS_HEADLESS 1)
set(TEXRENDER_HAS_SVG 1)
set(TEXRENDER_HAS_RAYLIB 0)
set(TEXRENDER_HAS_SDL2 0)
if (TEXRENDER_BACKEND STREQUAL "raylib")
    set(TEXRENDER_HAS_RAYLIB 1)
elseif (TEXRENDER_BACKEND STREQUAL "sdl2")
    set(TEXRENDER_HAS_SDL2 1)
endif ()

set(TEXRENDER_HAS_RAYLIB_CMAKE FALSE)
set(TEXRENDER_HAS_SDL2_CMAKE FALSE)
if (TEXRENDER_HAS_RAYLIB)
    set(TEXRENDER_HAS_RAYLIB_CMAKE TRUE)
endif ()

if (TEXRENDER_HAS_SDL2)
    set(TEXRENDER_HAS_SDL2_CMAKE TRUE)
endif ()

set(TEXRENDER_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
configure_file(
        cmake/texrenderConfig.hpp.in
        "${TEXRENDER_GENERATED_INCLUDE_DIR}/texrender/config.hpp"
        @ONLY
)
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
texrender_target_warnings(texrender)
target_compile_definitions(texrender PRIVATE TEXRENDER_BUILDING_LIBRARY)
get_target_property(_texrender_library_type texrender TYPE)
if (_texrender_library_type STREQUAL "SHARED_LIBRARY" OR _texrender_library_type STREQUAL "MODULE_LIBRARY")
    target_compile_definitions(texrender INTERFACE TEXRENDER_SHARED)
endif ()

unset(_texrender_library_type)
set_target_properties(texrender PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VERSION "${PROJECT_VERSION}"
        SOVERSION "${PROJECT_VERSION_MAJOR}"
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
if (NOT BUILD_SHARED_LIBS)
    target_link_libraries(microtex_core PRIVATE texrender)
endif ()

if (TEXRENDER_BACKEND_LIBS)
    target_link_libraries(texrender PUBLIC
            "$<BUILD_INTERFACE:${TEXRENDER_BACKEND_LIBS}>"
            "$<INSTALL_INTERFACE:${TEXRENDER_BACKEND_LIBS}>"
    )
endif ()

target_compile_features(texrender PRIVATE cxx_std_17)
target_compile_features(texrender INTERFACE cxx_std_17)
set(TEXRENDER_MACOS_DEPLOYMENT_LINK_FLAG "")
if (APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    set(TEXRENDER_MACOS_DEPLOYMENT_LINK_FLAG "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    target_link_options(texrender INTERFACE "${TEXRENDER_MACOS_DEPLOYMENT_LINK_FLAG}")
endif ()

if (TEXRENDER_BUILD_EXAMPLES)
    if (EMSCRIPTEN AND TEXRENDER_BACKEND STREQUAL "raylib")
        set(TEXRENDER_EXAMPLES texrender_live examples/live_edit.cpp)
    else ()
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
            list(APPEND TEXRENDER_EXAMPLES
                    texrender_sdl2_minimal examples/sdl2_minimal.cpp
            )
        endif ()
    endif ()

    while (TEXRENDER_EXAMPLES)
        list(POP_FRONT TEXRENDER_EXAMPLES example_name example_source)
        add_executable(${example_name} ${example_source})
        target_link_libraries(${example_name} PRIVATE texrender::texrender ${TEXRENDER_BACKEND_EXECUTABLE_LIBS})
        texrender_target_warnings(${example_name})
        set_target_properties(${example_name} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
        if (EMSCRIPTEN)
            target_compile_definitions(${example_name} PRIVATE TEXRENDER_RES_DIR="/res")
        else ()
            texrender_copy_resources(${example_name})
        endif ()
    endwhile ()

    if (TARGET texrender_live AND EMSCRIPTEN)
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
    endif ()
endif ()

if (TEXRENDER_BUILD_BENCHMARKS AND NOT EMSCRIPTEN)
    add_executable(texrender_render_benchmark benchmarks/render_benchmark.cpp)
    target_link_libraries(texrender_render_benchmark PRIVATE texrender::texrender)
    texrender_target_warnings(texrender_render_benchmark)
    target_compile_features(texrender_render_benchmark PRIVATE cxx_std_17)
    texrender_copy_resources(texrender_render_benchmark)
endif ()

if (TEXRENDER_BUILD_TESTS AND NOT EMSCRIPTEN)
    enable_testing()
    find_package(Threads REQUIRED)
    add_executable(texrender_transform2d_test tests/transform2d_test.cpp)
    target_include_directories(texrender_transform2d_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)
    texrender_target_warnings(texrender_transform2d_test)
    target_compile_features(texrender_transform2d_test PRIVATE cxx_std_17)
    add_test(NAME transform2d COMMAND texrender_transform2d_test)
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
    add_executable(texrender_headless_render_test tests/headless_render_test.cpp)
    target_link_libraries(texrender_headless_render_test PRIVATE texrender::texrender)
    texrender_target_warnings(texrender_headless_render_test)
    target_compile_features(texrender_headless_render_test PRIVATE cxx_std_17)
    target_compile_definitions(texrender_headless_render_test PRIVATE TEXRENDER_RES_DIR="${MICROTEX_RES_DIR}")
    add_test(NAME headless_render COMMAND texrender_headless_render_test)
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
    set(TEXRENDER_PUBLIC_HEADER_SELF_CONTAINMENT_HEADERS
            cache_options.hpp
            config.hpp
            draw_surface.hpp
            export.hpp
            formula_layout.hpp
            glyph_bitmap.hpp
            headless_surface.hpp
            latex_document.hpp
            render_handle.hpp
            svg_surface.hpp
            texrender.hpp
            version.hpp
    )
    if (TEXRENDER_BACKEND STREQUAL "raylib")
        list(APPEND TEXRENDER_PUBLIC_HEADER_SELF_CONTAINMENT_HEADERS raylib_surface.hpp)
    elseif (TEXRENDER_BACKEND STREQUAL "sdl2")
        list(APPEND TEXRENDER_PUBLIC_HEADER_SELF_CONTAINMENT_HEADERS sdl2_surface.hpp)
    endif ()

    foreach (header IN LISTS TEXRENDER_PUBLIC_HEADER_SELF_CONTAINMENT_HEADERS)
        string(REPLACE ".hpp" "" header_target_suffix "${header}")
        string(REPLACE "_" "-" header_test_suffix "${header_target_suffix}")
        set(header_source "${CMAKE_CURRENT_BINARY_DIR}/public-header-self-containment/${header_target_suffix}.cpp")
        file(WRITE "${header_source}" "#include <texrender/${header}>\n\nint main()\n{\n    return 0;\n}\n")
        add_executable("texrender_public_header_${header_target_suffix}_self_contained" "${header_source}")
        target_link_libraries("texrender_public_header_${header_target_suffix}_self_contained" PRIVATE texrender::texrender)
        texrender_target_warnings("texrender_public_header_${header_target_suffix}_self_contained")
        target_compile_features("texrender_public_header_${header_target_suffix}_self_contained" PRIVATE cxx_std_17)
        add_test(
                NAME "public_header_self_contained.${header_test_suffix}"
                COMMAND "texrender_public_header_${header_target_suffix}_self_contained"
        )
    endforeach ()

    if (TEXRENDER_BACKEND STREQUAL "raylib")
        add_executable(texrender_public_headers_raylib_cxx17 tests/public_headers_raylib_cxx17.cpp)
        target_link_libraries(texrender_public_headers_raylib_cxx17 PRIVATE texrender::texrender ${TEXRENDER_BACKEND_LIBS})
        texrender_target_warnings(texrender_public_headers_raylib_cxx17)
        target_compile_features(texrender_public_headers_raylib_cxx17 PRIVATE cxx_std_17)
        add_test(NAME public_headers_raylib_cxx17 COMMAND texrender_public_headers_raylib_cxx17)
    elseif (TEXRENDER_BACKEND STREQUAL "sdl2")
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
    endif ()

    add_test(NAME consumer_smoke
            COMMAND ${CMAKE_COMMAND}
            -DTEXRENDER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -DTEXRENDER_CONSUMER_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/consumer-smoke
            -DTEXRENDER_CONSUMER_CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DTEXRENDER_CONSUMER_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DTEXRENDER_CONSUMER_GENERATOR=${CMAKE_GENERATOR}
            -DTEXRENDER_TEST_CONFIG=$<CONFIG>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/consumer/run_consumer_test.cmake)
endif ()

if (TEXRENDER_BUILD_FUZZERS AND NOT EMSCRIPTEN)
    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "TEXRENDER_BUILD_FUZZERS requires Clang/libFuzzer")
    endif ()
    if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        message(FATAL_ERROR
                "TEXRENDER_BUILD_FUZZERS requires upstream LLVM Clang with libFuzzer. "
                "AppleClang may omit the fuzzer runtime; use Homebrew LLVM or Linux Clang.")
    endif ()

    add_executable(texrender_try_render_fuzzer tests/fuzz/try_render_fuzzer.cpp)
    target_link_libraries(texrender_try_render_fuzzer PRIVATE texrender::texrender)
    texrender_target_warnings(texrender_try_render_fuzzer)
    target_compile_features(texrender_try_render_fuzzer PRIVATE cxx_std_17)
    target_compile_definitions(texrender_try_render_fuzzer PRIVATE TEXRENDER_RES_DIR="${MICROTEX_RES_DIR}")
    target_compile_options(texrender_try_render_fuzzer PRIVATE -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer)
    target_link_options(texrender_try_render_fuzzer PRIVATE -fsanitize=fuzzer,address,undefined)
endif ()

set(_texrender_install_default ${PROJECT_IS_TOP_LEVEL})
if (TEXRENDER_TINYXML2_FETCHED)
    set(_texrender_install_default OFF)
endif ()

if (TEXRENDER_FREETYPE_FETCHED)
    set(_texrender_install_default OFF)
endif ()

if (TEXRENDER_RAYLIB_FETCHED)
    set(_texrender_install_default OFF)
endif ()

if (TEXRENDER_SDL2_FETCHED)
    set(_texrender_install_default OFF)
endif ()

option(TEXRENDER_INSTALL "Generate install and find_package(texrender) support" ${_texrender_install_default})
if (TEXRENDER_INSTALL AND TEXRENDER_TINYXML2_FETCHED)
    message(FATAL_ERROR
            "TEXRENDER_INSTALL=ON with a source-built tinyxml2 cannot be exported cleanly. "
            "Install tinyxml2 so find_package(tinyxml2 CONFIG) succeeds, or consume texrender with "
            "FetchContent/add_subdirectory instead of installing it.")
endif ()

if (TEXRENDER_INSTALL AND TEXRENDER_FREETYPE_FETCHED)
    message(FATAL_ERROR
            "TEXRENDER_INSTALL=ON with a source-built FreeType cannot be exported cleanly. "
            "Install FreeType so find_package(Freetype) succeeds, or consume texrender with "
            "FetchContent/add_subdirectory instead of installing it.")
endif ()

if (TEXRENDER_INSTALL AND TEXRENDER_RAYLIB_FETCHED)
    message(FATAL_ERROR
            "TEXRENDER_INSTALL=ON with a source-built raylib cannot be exported cleanly. "
            "Install raylib so find_package(raylib) succeeds, or install with -DTEXRENDER_BACKEND=headless "
            "or -DTEXRENDER_BACKEND=svg.")
endif ()

if (TEXRENDER_INSTALL AND TEXRENDER_SDL2_FETCHED)
    message(FATAL_ERROR
            "TEXRENDER_INSTALL=ON with a source-built SDL2 cannot be exported cleanly. "
            "Install SDL2 so find_package(SDL2) succeeds, or install with -DTEXRENDER_BACKEND=headless "
            "or -DTEXRENDER_BACKEND=svg.")
endif ()

if (TEXRENDER_INSTALL)
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
    if (TEXRENDER_BACKEND STREQUAL "raylib")
        list(APPEND TEXRENDER_PUBLIC_HEADERS include/texrender/raylib_surface.hpp)
    elseif (TEXRENDER_BACKEND STREQUAL "sdl2")
        list(APPEND TEXRENDER_PUBLIC_HEADERS include/texrender/sdl2_surface.hpp)
    endif ()

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
    if (TEXRENDER_BACKEND STREQUAL "raylib")
        string(APPEND TEXRENDER_PKG_CONFIG_REQUIRES_PRIVATE " raylib")
    elseif (TEXRENDER_BACKEND STREQUAL "sdl2")
        string(APPEND TEXRENDER_PKG_CONFIG_REQUIRES_PRIVATE " sdl2")
    endif ()

    set(TEXRENDER_PKG_CONFIG_LINK_FLAGS "")
    if (TEXRENDER_MACOS_DEPLOYMENT_LINK_FLAG)
        set(TEXRENDER_PKG_CONFIG_LINK_FLAGS " ${TEXRENDER_MACOS_DEPLOYMENT_LINK_FLAG}")
    endif ()

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
endif ()

if (PROJECT_IS_TOP_LEVEL AND NOT EMSCRIPTEN)
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
            "/\\\\.git$"
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
endif ()
