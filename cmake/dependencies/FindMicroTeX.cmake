set(_texrender_default_microtex_source_dir "${CMAKE_CURRENT_LIST_DIR}/../../third_party/microtex")
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

function(texrender_add_microtex_core source_dir)
    file(GLOB _microtex_core_sources CONFIGURE_DEPENDS
            "${source_dir}/atom/*.cpp"
            "${source_dir}/box/*.cpp"
            "${source_dir}/core/*.cpp"
            "${source_dir}/fonts/*.cpp"
            "${source_dir}/utils/*.cpp"
            "${source_dir}/res/builtin/*.cpp"
            "${source_dir}/res/font/*.cpp"
            "${source_dir}/res/parser/*.cpp"
            "${source_dir}/res/reg/*.cpp"
            "${source_dir}/res/sym/*.cpp"
            "${source_dir}/latex.cpp"
            "${source_dir}/render.cpp"
    )

    if (NOT _microtex_core_sources)
        message(FATAL_ERROR "No MicroTeX core sources found in '${source_dir}'")
    endif ()

    add_library(microtex_core STATIC ${_microtex_core_sources})
    add_library(microtex::core ALIAS microtex_core)
    target_include_directories(microtex_core SYSTEM PRIVATE "${source_dir}")
    target_compile_features(microtex_core PUBLIC cxx_std_17)
    target_link_libraries(microtex_core PRIVATE tinyxml2::tinyxml2)
endfunction()

texrender_add_microtex_core("${MICROTEX_SRC}")

if (TARGET microtex::core)
    set(MicroTeX_FOUND TRUE)
else ()
    set(MicroTeX_FOUND FALSE)
endif ()
