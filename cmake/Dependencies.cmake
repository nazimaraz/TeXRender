# ===========================================================================
# Dependencies for latex-renderer
#
#  * MicroTeX  -> source-only fetch; we compile just its platform-agnostic
#                 core into `microtex::core` and supply our own Graphics2D.
#  * tinyxml2  -> required by the MicroTeX core resource parsers.
#  * FreeType  -> used by our backend-neutral glyph rasteriser.
#  * raylib    -> the default GUI backend.
#  * SDL2      -> optional GUI backend proving the surface seam beyond raylib.
#
# Everything is resolved with find_package first, falling back to FetchContent
# so a fresh clone builds with zero manual setup.
# ===========================================================================
include(FetchContent)

# --- tinyxml2 --------------------------------------------------------------
find_package(tinyxml2 CONFIG QUIET)
if(NOT tinyxml2_FOUND)
    FetchContent_Declare(
        tinyxml2
        GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
        GIT_TAG 10.0.0
        GIT_SHALLOW TRUE
    )
    set(tinyxml2_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(tinyxml2)
endif()

# --- FreeType --------------------------------------------------------------
find_package(Freetype QUIET)
if(NOT Freetype_FOUND)
    FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG VER-2-13-2
        GIT_SHALLOW TRUE
    )
    # FreeType 2.13's cmake_minimum_required(3.0) is below CMake 4's floor;
    # this lets it configure under modern CMake without patching its sources.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    FetchContent_MakeAvailable(freetype)
    unset(CMAKE_POLICY_VERSION_MINIMUM)
    # Normalise to the imported-target name used throughout this project.
    if(NOT TARGET Freetype::Freetype)
        add_library(Freetype::Freetype ALIAS freetype)
    endif()
endif()

# --- raylib (only when its backend is selected) ----------------------------
set(TEXGUI_RAYLIB_FETCHED FALSE CACHE INTERNAL "Whether texgui fetched raylib instead of using find_package" FORCE)
if(TEXGUI_BACKEND STREQUAL "raylib")
    find_package(raylib QUIET)
    if(NOT raylib_FOUND)
        if(EMSCRIPTEN)
            # raylib's own platform switch: build its HTML5/WebGL backend.
            set(PLATFORM "Web" CACHE STRING "raylib platform" FORCE)
        endif()
        FetchContent_Declare(
            raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG 5.5
            GIT_SHALLOW TRUE
        )
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(raylib)
        set(TEXGUI_RAYLIB_FETCHED TRUE CACHE INTERNAL "Whether texgui fetched raylib instead of using find_package" FORCE)
    endif()
endif()

# --- SDL2 (only when its backend is selected) ------------------------------
set(TEXGUI_SDL2_FETCHED FALSE CACHE INTERNAL "Whether texgui fetched SDL2 instead of using find_package" FORCE)
set(TEXGUI_SDL2_TARGET "" CACHE INTERNAL "SDL2 target used by texgui" FORCE)
set(TEXGUI_SDL2MAIN_TARGET "" CACHE INTERNAL "Optional SDL2main target used by SDL2 examples" FORCE)
if(TEXGUI_BACKEND STREQUAL "sdl2")
    find_package(SDL2 CONFIG QUIET)
    if(NOT TARGET SDL2::SDL2)
        find_package(SDL2 QUIET)
    endif()

    if(NOT TARGET SDL2::SDL2 AND NOT TARGET SDL2::SDL2-static AND NOT TARGET SDL2)
        FetchContent_Declare(
            SDL2
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-2.32.10
            GIT_SHALLOW TRUE
        )
        set(SDL_TEST OFF CACHE BOOL "" FORCE)
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(SDL2)
        set(TEXGUI_SDL2_FETCHED TRUE CACHE INTERNAL "Whether texgui fetched SDL2 instead of using find_package" FORCE)
    endif()

    if(TARGET SDL2::SDL2)
        set(TEXGUI_SDL2_TARGET SDL2::SDL2 CACHE INTERNAL "SDL2 target used by texgui" FORCE)
    elseif(TARGET SDL2::SDL2-static)
        set(TEXGUI_SDL2_TARGET SDL2::SDL2-static CACHE INTERNAL "SDL2 target used by texgui" FORCE)
    elseif(TARGET SDL2)
        set(TEXGUI_SDL2_TARGET SDL2 CACHE INTERNAL "SDL2 target used by texgui" FORCE)
    else()
        message(FATAL_ERROR "TEXGUI_BACKEND=sdl2 requires an SDL2 CMake target")
    endif()

    if(TARGET SDL2::SDL2main)
        set(TEXGUI_SDL2MAIN_TARGET SDL2::SDL2main CACHE INTERNAL "Optional SDL2main target used by SDL2 examples" FORCE)
    endif()
endif()

# --- MicroTeX (download only; we build the core ourselves) -----------------
# SOURCE_SUBDIR points at a directory with no CMakeLists.txt, so MakeAvailable
# fetches the sources without running MicroTeX's own CMake (which would drag in
# Cairo/GTK on Unix). This is the supported replacement for the deprecated
# single-argument FetchContent_Populate (CMP0169).
FetchContent_Declare(
    microtex
    GIT_REPOSITORY https://github.com/NanoMichael/MicroTeX.git
    GIT_TAG 0e3707f6dafebb121d98b53c64364d16fefe481d
    SOURCE_SUBDIR do-not-run-microtex-cmake
)
FetchContent_MakeAvailable(microtex)

set(MICROTEX_SRC "${microtex_SOURCE_DIR}/src")
set(MICROTEX_RES_DIR "${microtex_SOURCE_DIR}/res" CACHE INTERNAL "MicroTeX resource directory")

# MicroTeX's IndexedArray binary search initializes the high bound to `_rows`
# instead of the last valid row. Misses can read one row past static font tables
# under ASan (for example accent next-larger lookups). Patch the fetched source
# in the build tree before compiling the pinned core.
set(_texgui_microtex_indexed_arr "${MICROTEX_SRC}/utils/indexed_arr.h")
file(READ "${_texgui_microtex_indexed_arr}" _texgui_microtex_indexed_arr_text)
set(_texgui_microtex_indexed_arr_before "int     l = 0, h = _rows;")
set(_texgui_microtex_indexed_arr_after "int     l = 0, h = static_cast<int>(_rows) - 1;")
if(_texgui_microtex_indexed_arr_text MATCHES "int     l = 0, h = _rows;")
    string(REPLACE
        "${_texgui_microtex_indexed_arr_before}"
        "${_texgui_microtex_indexed_arr_after}"
        _texgui_microtex_indexed_arr_text
        "${_texgui_microtex_indexed_arr_text}")
    file(WRITE "${_texgui_microtex_indexed_arr}" "${_texgui_microtex_indexed_arr_text}")
elseif(NOT _texgui_microtex_indexed_arr_text MATCHES "int     l = 0, h = static_cast<int>\\(_rows\\) - 1;")
    message(FATAL_ERROR "MicroTeX IndexedArray patch did not match ${_texgui_microtex_indexed_arr}")
endif()
unset(_texgui_microtex_indexed_arr)
unset(_texgui_microtex_indexed_arr_text)
unset(_texgui_microtex_indexed_arr_before)
unset(_texgui_microtex_indexed_arr_after)

# Authoritative core source list (mirrors MicroTeX's own target_sources, minus
# every src/platform/** backend -- we provide Graphics2D/Font/TextLayout).
set(MICROTEX_CORE_SOURCES
    atom/atom_basic.cpp atom/atom_char.cpp atom/atom_impl.cpp atom/atom_matrix.cpp
    atom/atom_row.cpp atom/atom_space.cpp atom/colors_def.cpp atom/unit_conversion.cpp
    box/box.cpp box/box_factory.cpp box/box_group.cpp box/box_single.cpp
    core/core.cpp core/formula.cpp core/formula_def.cpp core/glue.cpp
    core/localized_num.cpp core/macro.cpp core/macro_def.cpp core/macro_impl.cpp core/parser.cpp
    fonts/alphabet.cpp fonts/font_basic.cpp fonts/font_info.cpp fonts/fonts.cpp
    utils/string_utils.cpp utils/utf.cpp utils/utils.cpp
    res/builtin/formula_mappings.res.cpp res/builtin/symbol_mapping.res.cpp
    res/builtin/tex_param.res.cpp res/builtin/tex_symbols.res.cpp
    res/font/bi10.def.cpp res/font/bx10.def.cpp res/font/cmbsy10.def.cpp res/font/cmbx10.def.cpp
    res/font/cmbxti10.def.cpp res/font/cmex10.def.cpp res/font/cmmi10.def.cpp
    res/font/cmmi10_unchanged.def.cpp res/font/cmmib10.def.cpp res/font/cmmib10_unchanged.def.cpp
    res/font/cmr10.def.cpp res/font/cmss10.def.cpp res/font/cmssbx10.def.cpp res/font/cmssi10.def.cpp
    res/font/cmsy10.def.cpp res/font/cmti10.def.cpp res/font/cmti10_unchanged.def.cpp
    res/font/cmtt10.def.cpp res/font/dsrom10.def.cpp res/font/eufb10.def.cpp res/font/eufm10.def.cpp
    res/font/i10.def.cpp res/font/moustache.def.cpp res/font/msam10.def.cpp res/font/msbm10.def.cpp
    res/font/r10.def.cpp res/font/r10_unchanged.def.cpp res/font/rsfs10.def.cpp res/font/sb10.def.cpp
    res/font/sbi10.def.cpp res/font/si10.def.cpp res/font/special.def.cpp res/font/ss10.def.cpp
    res/font/stmary10.def.cpp res/font/tt10.def.cpp
    res/parser/font_parser.cpp res/parser/formula_parser.cpp
    res/reg/builtin_font_reg.cpp res/reg/builtin_syms_reg.cpp
    res/sym/amsfonts.def.cpp res/sym/amssymb.def.cpp res/sym/base.def.cpp
    res/sym/stmaryrd.def.cpp res/sym/symspecial.def.cpp
    latex.cpp render.cpp
)
list(TRANSFORM MICROTEX_CORE_SOURCES PREPEND "${MICROTEX_SRC}/")

add_library(microtex_core STATIC ${MICROTEX_CORE_SOURCES})
add_library(microtex::core ALIAS microtex_core)

target_include_directories(microtex_core SYSTEM PRIVATE "${MICROTEX_SRC}")
# Pin MicroTeX's own sources to C++17 (its legacy code predates C++23 removals);
# the PUBLIC feature still raises consumers to at least 17.
target_compile_features(microtex_core PUBLIC cxx_std_17)
set_target_properties(microtex_core PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
set_target_properties(microtex_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# tinyxml2 exports either an imported target or a bare `tinyxml2` target.
if(TARGET tinyxml2::tinyxml2)
    target_link_libraries(microtex_core PRIVATE tinyxml2::tinyxml2)
else()
    target_link_libraries(microtex_core PRIVATE tinyxml2)
endif()

# No BUILD_GTK/BUILD_QT/... so no platform graphic_*.cpp compiles.
# No HAVE_LOG and no GRAPHICS_DEBUG so rendering stays clean and quiet.
# MicroTeX's source has accumulated warnings; keep them out of our face.
if(MSVC)
    target_compile_options(microtex_core PRIVATE /w)
else()
    target_compile_options(microtex_core PRIVATE -w)
endif()
