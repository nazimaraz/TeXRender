include(FetchContent)
function(texrender_declare_source_dependency name upper submodule_dir marker)
    if (TEXRENDER_USE_SUBMODULE_DEPENDENCIES AND EXISTS "${submodule_dir}/${marker}")
        FetchContent_Declare(${name} SOURCE_DIR "${submodule_dir}")
        set(TEXRENDER_${upper}_SOURCE_KIND "submodule" CACHE INTERNAL "How texrender resolved ${name}" FORCE)
    else ()
        message(FATAL_ERROR
                "${name} was not found as an installed package and no usable source checkout exists at "
                "'${submodule_dir}'. Initialize submodules with `git submodule update --init --recursive`, ")
    endif ()
endfunction()

set(TEXRENDER_TINYXML2_FETCHED FALSE CACHE INTERNAL "Whether texrender fetched tinyxml2 instead of using find_package" FORCE)
set(TEXRENDER_TINYXML2_SOURCE_KIND "package" CACHE INTERNAL "How texrender resolved tinyxml2" FORCE)
find_package(tinyxml2 CONFIG QUIET)
if (NOT tinyxml2_FOUND)
    texrender_declare_source_dependency(
            tinyxml2
            TINYXML2
            "${CMAKE_CURRENT_LIST_DIR}/../third_party/tinyxml2"
            CMakeLists.txt
    )
    set(tinyxml2_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(tinyxml2)
    set(TEXRENDER_TINYXML2_FETCHED TRUE CACHE INTERNAL "Whether texrender fetched tinyxml2 instead of using find_package" FORCE)
endif ()

set(TEXRENDER_FREETYPE_FETCHED FALSE CACHE INTERNAL "Whether texrender fetched FreeType instead of using find_package" FORCE)
set(TEXRENDER_FREETYPE_SOURCE_KIND "package" CACHE INTERNAL "How texrender resolved FreeType" FORCE)
find_package(Freetype QUIET)
if (NOT Freetype_FOUND)
    texrender_declare_source_dependency(
            freetype
            FREETYPE
            "${CMAKE_CURRENT_LIST_DIR}/../third_party/freetype"
            CMakeLists.txt
    )
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    FetchContent_MakeAvailable(freetype)
    unset(CMAKE_POLICY_VERSION_MINIMUM)
    if (NOT TARGET Freetype::Freetype)
        add_library(Freetype::Freetype ALIAS freetype)
    endif ()

    set(TEXRENDER_FREETYPE_FETCHED TRUE CACHE INTERNAL "Whether texrender fetched FreeType instead of using find_package" FORCE)
endif ()

set(TEXRENDER_RAYLIB_FETCHED FALSE CACHE INTERNAL "Whether texrender fetched raylib instead of using find_package" FORCE)
set(TEXRENDER_RAYLIB_SOURCE_KIND "package" CACHE INTERNAL "How texrender resolved raylib" FORCE)
if (TEXRENDER_BACKEND STREQUAL "raylib")
    find_package(raylib QUIET)
    if (NOT raylib_FOUND)
        if (EMSCRIPTEN)
            # raylib's own platform switch: build its HTML5/WebGL backend.
            set(PLATFORM "Web" CACHE STRING "raylib platform" FORCE)
        endif ()

        texrender_declare_source_dependency(
                raylib
                RAYLIB
                "${CMAKE_CURRENT_LIST_DIR}/../third_party/raylib"
                CMakeLists.txt
        )
        set(_texrender_had_raylib_build_examples_cache FALSE)
        if (DEFINED CACHE{BUILD_EXAMPLES})
            set(_texrender_had_raylib_build_examples_cache TRUE)
            get_property(_texrender_saved_build_examples CACHE BUILD_EXAMPLES PROPERTY VALUE)
            get_property(_texrender_saved_build_examples_type CACHE BUILD_EXAMPLES PROPERTY TYPE)
            get_property(_texrender_saved_build_examples_help CACHE BUILD_EXAMPLES PROPERTY HELPSTRING)
        endif ()

        set(_texrender_had_cmake_warn_deprecated FALSE)
        if (DEFINED CMAKE_WARN_DEPRECATED)
            set(_texrender_had_cmake_warn_deprecated TRUE)
            set(_texrender_saved_cmake_warn_deprecated "${CMAKE_WARN_DEPRECATED}")
        endif ()

        set(_texrender_had_cmake_warn_deprecated_cache FALSE)
        if (DEFINED CACHE{CMAKE_WARN_DEPRECATED})
            set(_texrender_had_cmake_warn_deprecated_cache TRUE)
            get_property(_texrender_saved_cmake_warn_deprecated_cache CACHE CMAKE_WARN_DEPRECATED PROPERTY VALUE)
            get_property(_texrender_saved_cmake_warn_deprecated_type CACHE CMAKE_WARN_DEPRECATED PROPERTY TYPE)
            get_property(_texrender_saved_cmake_warn_deprecated_help CACHE CMAKE_WARN_DEPRECATED PROPERTY HELPSTRING)
        endif ()

        set(_texrender_had_cmake_suppress_developer_warnings FALSE)
        if (DEFINED CMAKE_SUPPRESS_DEVELOPER_WARNINGS)
            set(_texrender_had_cmake_suppress_developer_warnings TRUE)
            set(_texrender_saved_cmake_suppress_developer_warnings "${CMAKE_SUPPRESS_DEVELOPER_WARNINGS}")
        endif ()

        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
        set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON)
        FetchContent_MakeAvailable(raylib)
        if (_texrender_had_cmake_warn_deprecated_cache)
            if (_texrender_saved_cmake_warn_deprecated_type STREQUAL "UNINITIALIZED")
                set(_texrender_saved_cmake_warn_deprecated_type BOOL)
            endif ()

            set(CMAKE_WARN_DEPRECATED "${_texrender_saved_cmake_warn_deprecated_cache}" CACHE
                    "${_texrender_saved_cmake_warn_deprecated_type}" "${_texrender_saved_cmake_warn_deprecated_help}" FORCE)
        else ()
            unset(CMAKE_WARN_DEPRECATED CACHE)
        endif ()

        if (_texrender_had_cmake_warn_deprecated)
            set(CMAKE_WARN_DEPRECATED "${_texrender_saved_cmake_warn_deprecated}")
        else ()
            unset(CMAKE_WARN_DEPRECATED)
        endif ()

        if (_texrender_had_cmake_suppress_developer_warnings)
            set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS "${_texrender_saved_cmake_suppress_developer_warnings}")
        else ()
            unset(CMAKE_SUPPRESS_DEVELOPER_WARNINGS)
        endif ()

        if (_texrender_had_raylib_build_examples_cache)
            if (_texrender_saved_build_examples_type STREQUAL "UNINITIALIZED")
                set(_texrender_saved_build_examples_type BOOL)
            endif ()

            set(BUILD_EXAMPLES "${_texrender_saved_build_examples}" CACHE "${_texrender_saved_build_examples_type}"
                    "${_texrender_saved_build_examples_help}" FORCE)
        else ()
            unset(BUILD_EXAMPLES CACHE)
        endif ()

        unset(_texrender_had_raylib_build_examples_cache)
        unset(_texrender_saved_build_examples)
        unset(_texrender_saved_build_examples_type)
        unset(_texrender_saved_build_examples_help)
        unset(_texrender_had_cmake_warn_deprecated)
        unset(_texrender_saved_cmake_warn_deprecated)
        unset(_texrender_had_cmake_warn_deprecated_cache)
        unset(_texrender_saved_cmake_warn_deprecated_cache)
        unset(_texrender_saved_cmake_warn_deprecated_type)
        unset(_texrender_saved_cmake_warn_deprecated_help)
        unset(_texrender_had_cmake_suppress_developer_warnings)
        unset(_texrender_saved_cmake_suppress_developer_warnings)
        set(TEXRENDER_RAYLIB_FETCHED TRUE CACHE INTERNAL "Whether texrender fetched raylib instead of using find_package" FORCE)
    endif ()
endif ()

set(TEXRENDER_SDL2_FETCHED FALSE CACHE INTERNAL "Whether texrender fetched SDL2 instead of using find_package" FORCE)
set(TEXRENDER_SDL2_SOURCE_KIND "package" CACHE INTERNAL "How texrender resolved SDL2" FORCE)
set(TEXRENDER_SDL2_TARGET "" CACHE INTERNAL "SDL2 target used by texrender" FORCE)
set(TEXRENDER_SDL2MAIN_TARGET "" CACHE INTERNAL "Optional SDL2main target used by SDL2 examples" FORCE)
if (TEXRENDER_BACKEND STREQUAL "sdl2")
    find_package(SDL2 CONFIG QUIET)
    if (NOT TARGET SDL2::SDL2)
        find_package(SDL2 QUIET)
    endif ()

    if (NOT TARGET SDL2::SDL2 AND NOT TARGET SDL2::SDL2-static AND NOT TARGET SDL2)
        texrender_declare_source_dependency(
                SDL2
                SDL2
                "${CMAKE_CURRENT_LIST_DIR}/../third_party/sdl2"
                CMakeLists.txt
        )
        set(SDL_TEST OFF CACHE BOOL "" FORCE)
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(SDL2)
        set(TEXRENDER_SDL2_FETCHED TRUE CACHE INTERNAL "Whether texrender fetched SDL2 instead of using find_package" FORCE)
    endif ()

    if (TARGET SDL2::SDL2)
        set(TEXRENDER_SDL2_TARGET SDL2::SDL2 CACHE INTERNAL "SDL2 target used by texrender" FORCE)
    elseif (TARGET SDL2::SDL2-static)
        set(TEXRENDER_SDL2_TARGET SDL2::SDL2-static CACHE INTERNAL "SDL2 target used by texrender" FORCE)
    elseif (TARGET SDL2)
        set(TEXRENDER_SDL2_TARGET SDL2 CACHE INTERNAL "SDL2 target used by texrender" FORCE)
    else ()
        message(FATAL_ERROR "TEXRENDER_BACKEND=sdl2 requires an SDL2 CMake target")
    endif ()

    if (TARGET SDL2::SDL2main)
        set(TEXRENDER_SDL2MAIN_TARGET SDL2::SDL2main CACHE INTERNAL "Optional SDL2main target used by SDL2 examples" FORCE)
    endif ()
endif ()

set(_texrender_default_microtex_source_dir "${CMAKE_CURRENT_LIST_DIR}/../third_party/microtex")
if (DEFINED FETCHCONTENT_SOURCE_DIR_MICROTEX AND NOT FETCHCONTENT_SOURCE_DIR_MICROTEX STREQUAL "")
    set(_texrender_default_microtex_source_dir "${FETCHCONTENT_SOURCE_DIR_MICROTEX}")
endif ()

set(TEXRENDER_MICROTEX_SOURCE_DIR "${_texrender_default_microtex_source_dir}" CACHE PATH
        "Path to the MicroTeX source checkout used by texrender")
set(_texrender_microtex_source_dir "${TEXRENDER_MICROTEX_SOURCE_DIR}")
if (NOT IS_ABSOLUTE "${_texrender_microtex_source_dir}")
    get_filename_component(_texrender_microtex_source_dir "${_texrender_microtex_source_dir}" ABSOLUTE
            BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()

if (NOT EXISTS "${_texrender_microtex_source_dir}/src/latex.cpp"
        OR NOT EXISTS "${_texrender_microtex_source_dir}/res/.clatexmath-res_root")
    message(FATAL_ERROR
            "MicroTeX sources were not found at '${_texrender_microtex_source_dir}'. "
            "Initialize the submodule with `git submodule update --init --recursive`, "
            "or configure with -DTEXRENDER_MICROTEX_SOURCE_DIR=/path/to/MicroTeX.")
endif ()

set(_texrender_microtex_build_source_dir "${CMAKE_CURRENT_BINARY_DIR}/_deps/microtex-src")
file(REMOVE_RECURSE "${_texrender_microtex_build_source_dir}")
file(MAKE_DIRECTORY "${_texrender_microtex_build_source_dir}")
file(COPY
        "${_texrender_microtex_source_dir}/src"
        "${_texrender_microtex_source_dir}/res"
        DESTINATION "${_texrender_microtex_build_source_dir}"
)
set(MICROTEX_SRC "${_texrender_microtex_build_source_dir}/src")
set(MICROTEX_RES_DIR "${_texrender_microtex_build_source_dir}/res" CACHE INTERNAL "MicroTeX resource directory")
unset(_texrender_default_microtex_source_dir)
unset(_texrender_microtex_source_dir)
unset(_texrender_microtex_build_source_dir)
set(_texrender_microtex_indexed_arr "${MICROTEX_SRC}/utils/indexed_arr.h")
file(READ "${_texrender_microtex_indexed_arr}" _texrender_microtex_indexed_arr_text)
set(_texrender_microtex_indexed_arr_before "int     l = 0, h = _rows;")
set(_texrender_microtex_indexed_arr_after "int     l = 0, h = static_cast<int>(_rows) - 1;")
if (_texrender_microtex_indexed_arr_text MATCHES "int     l = 0, h = _rows;")
    string(REPLACE
            "${_texrender_microtex_indexed_arr_before}"
            "${_texrender_microtex_indexed_arr_after}"
            _texrender_microtex_indexed_arr_text
            "${_texrender_microtex_indexed_arr_text}")
    file(WRITE "${_texrender_microtex_indexed_arr}" "${_texrender_microtex_indexed_arr_text}")
elseif (NOT _texrender_microtex_indexed_arr_text MATCHES "int     l = 0, h = static_cast<int>\\(_rows\\) - 1;")
    message(FATAL_ERROR "MicroTeX IndexedArray patch did not match ${_texrender_microtex_indexed_arr}")
endif ()

unset(_texrender_microtex_indexed_arr)
unset(_texrender_microtex_indexed_arr_text)
unset(_texrender_microtex_indexed_arr_before)
unset(_texrender_microtex_indexed_arr_after)
set(_texrender_microtex_atom_basic "${MICROTEX_SRC}/atom/atom_basic.cpp")
file(READ "${_texrender_microtex_atom_basic}" _texrender_microtex_atom_basic_text)
set(_texrender_microtex_atom_basic_before "    auto box = acc->_base->createBox(*(env.crampStyle()));")
set(_texrender_microtex_atom_basic_after [[    auto box = (
      acc->_base == nullptr
      ? sptrOf<StrutBox>(0.f, 0.f, 0.f, 0.f)
      : acc->_base->createBox(*(env.crampStyle()))
    );]])
if (_texrender_microtex_atom_basic_text MATCHES "auto box = acc->_base->createBox\\(\\*\\(env\\.crampStyle\\(\\)\\)\\);")
    string(REPLACE
            "${_texrender_microtex_atom_basic_before}"
            "${_texrender_microtex_atom_basic_after}"
            _texrender_microtex_atom_basic_text
            "${_texrender_microtex_atom_basic_text}")
    file(WRITE "${_texrender_microtex_atom_basic}" "${_texrender_microtex_atom_basic_text}")
elseif (NOT _texrender_microtex_atom_basic_text MATCHES "acc->_base == nullptr[ \n\r\t]+\\?[ \n\r\t]+sptrOf<StrutBox>\\(0\\.f, 0\\.f, 0\\.f, 0\\.f\\)")
    message(FATAL_ERROR "MicroTeX ScriptsAtom accent patch did not match ${_texrender_microtex_atom_basic}")
endif ()

unset(_texrender_microtex_atom_basic)
unset(_texrender_microtex_atom_basic_text)
unset(_texrender_microtex_atom_basic_before)
unset(_texrender_microtex_atom_basic_after)
set(_texrender_microtex_atom_basic "${MICROTEX_SRC}/atom/atom_basic.cpp")
file(READ "${_texrender_microtex_atom_basic}" _texrender_microtex_atom_basic_text)
set(_texrender_microtex_atom_basic_before "  auto box = _base->createBox(e);")
set(_texrender_microtex_atom_basic_after [[  auto box = (
    _base == nullptr
    ? sptrOf<StrutBox>(0.f, 0.f, 0.f, 0.f)
    : _base->createBox(e)
  );]])
if (_texrender_microtex_atom_basic_text MATCHES "auto box = _base->createBox\\(e\\);")
    string(REPLACE
            "${_texrender_microtex_atom_basic_before}"
            "${_texrender_microtex_atom_basic_after}"
            _texrender_microtex_atom_basic_text
            "${_texrender_microtex_atom_basic_text}")
    file(WRITE "${_texrender_microtex_atom_basic}" "${_texrender_microtex_atom_basic_text}")
elseif (NOT _texrender_microtex_atom_basic_text MATCHES "_base == nullptr[ \n\r\t]+\\?[ \n\r\t]+sptrOf<StrutBox>\\(0\\.f, 0\\.f, 0\\.f, 0\\.f\\)[ \n\r\t]+:[ \n\r\t]+_base->createBox\\(e\\)")
    message(FATAL_ERROR "MicroTeX MathAtom null-base patch did not match ${_texrender_microtex_atom_basic}")
endif ()

unset(_texrender_microtex_atom_basic)
unset(_texrender_microtex_atom_basic_text)
unset(_texrender_microtex_atom_basic_before)
unset(_texrender_microtex_atom_basic_after)
set(_texrender_microtex_atom_impl "${MICROTEX_SRC}/atom/atom_impl.cpp")
file(READ "${_texrender_microtex_atom_impl}" _texrender_microtex_atom_impl_text)
set(_texrender_microtex_atom_impl_before [[  _type = base->_type;
  _base = base;]])
set(_texrender_microtex_atom_impl_after [[  _type = base == nullptr ? AtomType::ordinary : base->_type;
  _base = base;]])
if (_texrender_microtex_atom_impl_text MATCHES "_type = base->_type;")
    string(REPLACE
            "${_texrender_microtex_atom_impl_before}"
            "${_texrender_microtex_atom_impl_after}"
            _texrender_microtex_atom_impl_text
            "${_texrender_microtex_atom_impl_text}")
elseif (NOT _texrender_microtex_atom_impl_text MATCHES "_type = base == nullptr \\? AtomType::ordinary : base->_type;")
    message(FATAL_ERROR "MicroTeX RotateAtom constructor patch did not match ${_texrender_microtex_atom_impl}")
endif ()

set(_texrender_microtex_atom_impl_before "_base->createBox(env)")
set(_texrender_microtex_atom_impl_after "(_base == nullptr ? sptrOf<StrutBox>(0.f, 0.f, 0.f, 0.f) : _base->createBox(env))")
if (_texrender_microtex_atom_impl_text MATCHES "_base->createBox\\(env\\), _angle")
    string(REPLACE
            "${_texrender_microtex_atom_impl_before}"
            "${_texrender_microtex_atom_impl_after}"
            _texrender_microtex_atom_impl_text
            "${_texrender_microtex_atom_impl_text}")
    file(WRITE "${_texrender_microtex_atom_impl}" "${_texrender_microtex_atom_impl_text}")
elseif (NOT _texrender_microtex_atom_impl_text MATCHES "_base == nullptr \\? sptrOf<StrutBox>\\(0\\.f, 0\\.f, 0\\.f, 0\\.f\\) : _base->createBox\\(env\\)")
    message(FATAL_ERROR "MicroTeX RotateAtom createBox patch did not match ${_texrender_microtex_atom_impl}")
endif ()

unset(_texrender_microtex_atom_impl)
unset(_texrender_microtex_atom_impl_text)
unset(_texrender_microtex_atom_impl_before)
unset(_texrender_microtex_atom_impl_after)
set(_texrender_microtex_atom_basic_h "${MICROTEX_SRC}/atom/atom_basic.h")
file(READ "${_texrender_microtex_atom_basic_h}" _texrender_microtex_atom_basic_h_text)
set(_texrender_microtex_atom_basic_h_before [[  AtomType leftType() const override {
    return _base->leftType();
  }

  AtomType rightType() const override {
    return _base->rightType();
  }]])
set(_texrender_microtex_atom_basic_h_after [[  AtomType leftType() const override {
    return _base == nullptr ? AtomType::ordinary : _base->leftType();
  }

  AtomType rightType() const override {
    return _base == nullptr ? AtomType::ordinary : _base->rightType();
  }]])
if (_texrender_microtex_atom_basic_h_text MATCHES "return _base->leftType\\(\\);")
    string(REPLACE
            "${_texrender_microtex_atom_basic_h_before}"
            "${_texrender_microtex_atom_basic_h_after}"
            _texrender_microtex_atom_basic_h_text
            "${_texrender_microtex_atom_basic_h_text}")
    file(WRITE "${_texrender_microtex_atom_basic_h}" "${_texrender_microtex_atom_basic_h_text}")
elseif (NOT _texrender_microtex_atom_basic_h_text MATCHES "_base == nullptr \\? AtomType::ordinary : _base->leftType\\(\\)")
    message(FATAL_ERROR "MicroTeX UnderOverAtom type patch did not match ${_texrender_microtex_atom_basic_h}")
endif ()
unset(_texrender_microtex_atom_basic_h)
unset(_texrender_microtex_atom_basic_h_text)
unset(_texrender_microtex_atom_basic_h_before)
unset(_texrender_microtex_atom_basic_h_after)
set(_texrender_microtex_atom_basic "${MICROTEX_SRC}/atom/atom_basic.cpp")
file(READ "${_texrender_microtex_atom_basic}" _texrender_microtex_atom_basic_text)
set(_texrender_microtex_atom_basic_before [[  if (
    _base->_limitsType == LimitsType::noLimits
      || (_base->_limitsType == LimitsType::normal && style >= TexStyle::text)
    ) {]])
set(_texrender_microtex_atom_basic_after [[  if (
    _base != nullptr
      && (_base->_limitsType == LimitsType::noLimits
        || (_base->_limitsType == LimitsType::normal && style >= TexStyle::text))
    ) {]])
if (_texrender_microtex_atom_basic_text MATCHES "_base->_limitsType == LimitsType::noLimits")
    string(REPLACE
            "${_texrender_microtex_atom_basic_before}"
            "${_texrender_microtex_atom_basic_after}"
            _texrender_microtex_atom_basic_text
            "${_texrender_microtex_atom_basic_text}")
    file(WRITE "${_texrender_microtex_atom_basic}" "${_texrender_microtex_atom_basic_text}")
elseif (NOT _texrender_microtex_atom_basic_text MATCHES "_base != nullptr[ \n\r\t]+&& \\(_base->_limitsType == LimitsType::noLimits")
    message(FATAL_ERROR "MicroTeX UnderOverAtom limits patch did not match ${_texrender_microtex_atom_basic}")
endif ()
unset(_texrender_microtex_atom_basic)
unset(_texrender_microtex_atom_basic_text)
unset(_texrender_microtex_atom_basic_before)
unset(_texrender_microtex_atom_basic_after)
set(_texrender_microtex_atom_impl_h "${MICROTEX_SRC}/atom/atom_impl.h")
file(READ "${_texrender_microtex_atom_impl_h}" _texrender_microtex_atom_impl_h_text)
set(_texrender_microtex_atom_impl_h_before "    auto b = _base->createBox(env);")
set(_texrender_microtex_atom_impl_h_after "    auto b = _base != nullptr ? _base->createBox(env) : sptrOf<StrutBox>(0.f, 0.f, 0.f, 0.f);")
if (_texrender_microtex_atom_impl_h_text MATCHES "auto b = _base->createBox\\(env\\);")
    string(REPLACE
            "${_texrender_microtex_atom_impl_h_before}"
            "${_texrender_microtex_atom_impl_h_after}"
            _texrender_microtex_atom_impl_h_text
            "${_texrender_microtex_atom_impl_h_text}")
    file(WRITE "${_texrender_microtex_atom_impl_h}" "${_texrender_microtex_atom_impl_h_text}")
elseif (NOT _texrender_microtex_atom_impl_h_text MATCHES "auto b = _base != nullptr \\? _base->createBox\\(env\\) : sptrOf<StrutBox>\\(0\\.f, 0\\.f, 0\\.f, 0\\.f\\);")
    message(FATAL_ERROR "MicroTeX CedillaAtom null-base patch did not match ${_texrender_microtex_atom_impl_h}")
endif ()
unset(_texrender_microtex_atom_impl_h)
unset(_texrender_microtex_atom_impl_h_text)
unset(_texrender_microtex_atom_impl_h_before)
unset(_texrender_microtex_atom_impl_h_after)
set(_texrender_microtex_atom_impl_h "${MICROTEX_SRC}/atom/atom_impl.h")
file(READ "${_texrender_microtex_atom_impl_h}" _texrender_microtex_atom_impl_h_text)
set(_texrender_microtex_atom_impl_h_original "${_texrender_microtex_atom_impl_h_text}")
foreach (_texrender_microtex_atom_impl_h_var IN ITEMS box base bbase bbox)
    set(_texrender_microtex_atom_impl_h_before "    auto ${_texrender_microtex_atom_impl_h_var} = _base->createBox(env);")
    set(_texrender_microtex_atom_impl_h_after
            "    auto ${_texrender_microtex_atom_impl_h_var} = _base != nullptr ? _base->createBox(env) : sptrOf<StrutBox>(0.f, 0.f, 0.f, 0.f);")
    string(REPLACE
            "${_texrender_microtex_atom_impl_h_before}"
            "${_texrender_microtex_atom_impl_h_after}"
            _texrender_microtex_atom_impl_h_text
            "${_texrender_microtex_atom_impl_h_text}")
endforeach ()
if (_texrender_microtex_atom_impl_h_text MATCHES "_base->createBox\\(env\\);")
    message(FATAL_ERROR "MicroTeX wrapper null-base patch left direct _base->createBox(env) in ${_texrender_microtex_atom_impl_h}")
endif ()
if (NOT _texrender_microtex_atom_impl_h_text STREQUAL _texrender_microtex_atom_impl_h_original)
    file(WRITE "${_texrender_microtex_atom_impl_h}" "${_texrender_microtex_atom_impl_h_text}")
endif ()
unset(_texrender_microtex_atom_impl_h)
unset(_texrender_microtex_atom_impl_h_text)
unset(_texrender_microtex_atom_impl_h_original)
unset(_texrender_microtex_atom_impl_h_var)
unset(_texrender_microtex_atom_impl_h_before)
unset(_texrender_microtex_atom_impl_h_after)
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
target_compile_features(microtex_core PUBLIC cxx_std_17)
target_link_libraries(microtex_core PRIVATE tinyxml2::tinyxml2)
