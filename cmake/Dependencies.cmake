# ===========================================================================
# Dependencies for latex-renderer
#
#  * MicroTeX  -> source-only fetch; we compile just its platform-agnostic
#                 core into `microtex::core` and supply our own Graphics2D.
#  * tinyxml2  -> required by the MicroTeX core resource parsers.
#  * FreeType  -> used by our backend-neutral glyph rasteriser.
#  * raylib    -> the (currently only) GUI backend.
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
    FetchContent_MakeAvailable(freetype)
    # Normalise to the imported-target name used throughout this project.
    if(NOT TARGET Freetype::Freetype)
        add_library(Freetype::Freetype ALIAS freetype)
    endif()
endif()

# --- raylib (only when its backend is selected) ----------------------------
if(TEXGUI_BACKEND STREQUAL "raylib")
    find_package(raylib QUIET)
    if(NOT raylib_FOUND)
        FetchContent_Declare(
            raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG 5.5
            GIT_SHALLOW TRUE
        )
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(raylib)
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

target_include_directories(microtex_core PUBLIC "${MICROTEX_SRC}")
# Pin MicroTeX's own sources to C++17 (its legacy code predates C++23 removals);
# the PUBLIC feature still raises consumers to at least 17.
target_compile_features(microtex_core PUBLIC cxx_std_17)
set_target_properties(microtex_core PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

# tinyxml2 exports either an imported target or a bare `tinyxml2` target.
if(TARGET tinyxml2::tinyxml2)
    target_link_libraries(microtex_core PUBLIC tinyxml2::tinyxml2)
else()
    target_link_libraries(microtex_core PUBLIC tinyxml2)
endif()

# No BUILD_GTK/BUILD_QT/... so no platform graphic_*.cpp compiles.
# No HAVE_LOG and no GRAPHICS_DEBUG so rendering stays clean and quiet.
# MicroTeX's source has accumulated warnings; keep them out of our face.
if(MSVC)
    target_compile_options(microtex_core PRIVATE /w)
else()
    target_compile_options(microtex_core PRIVATE -w)
endif()
