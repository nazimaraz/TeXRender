set(TEXRENDER_IMGUI_TARGET "" CACHE INTERNAL "Dear ImGui target used by texrender" FORCE)
set(TEXRENDER_IMGUI_SOURCE_DIR "" CACHE INTERNAL "Dear ImGui source directory used by texrender" FORCE)
set(_texrender_imgui_find_required "${ImGui_FIND_REQUIRED}")
set(ImGui_FIND_REQUIRED FALSE)
find_package(imgui CONFIG QUIET)
set(ImGui_FIND_REQUIRED "${_texrender_imgui_find_required}")
unset(_texrender_imgui_find_required)

# Dear ImGui ships no build system, so the submodule is compiled here instead of via add_subdirectory.
if (NOT TARGET imgui::imgui AND NOT TARGET imgui)
    set(_texrender_imgui_dir "${CMAKE_CURRENT_LIST_DIR}/../../third_party/imgui")
    if (NOT TEXRENDER_USE_SUBMODULE_DEPENDENCIES)
        message(FATAL_ERROR
                "Dear ImGui was not found as an installed package. "
                "Install Dear ImGui, or enable TEXRENDER_USE_SUBMODULE_DEPENDENCIES.")
    endif ()

    if (NOT EXISTS "${_texrender_imgui_dir}/imgui.cpp")
        message(FATAL_ERROR
                "Dear ImGui was not found as an installed package and no usable source checkout exists at "
                "'${_texrender_imgui_dir}'. Initialize submodules with `git submodule update --init --recursive`.")
    endif ()

    add_library(imgui STATIC
            "${_texrender_imgui_dir}/imgui.cpp"
            "${_texrender_imgui_dir}/imgui_draw.cpp"
            "${_texrender_imgui_dir}/imgui_tables.cpp"
            "${_texrender_imgui_dir}/imgui_widgets.cpp"
    )
    target_include_directories(imgui SYSTEM PUBLIC "${_texrender_imgui_dir}")
    target_compile_features(imgui PUBLIC cxx_std_17)
    set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set(TEXRENDER_IMGUI_SOURCE_KIND "submodule" CACHE INTERNAL "How texrender resolved Dear ImGui" FORCE)
    set(TEXRENDER_IMGUI_SOURCE_DIR "${_texrender_imgui_dir}" CACHE INTERNAL "Dear ImGui source directory used by texrender" FORCE)
    unset(_texrender_imgui_dir)
endif ()

if (TARGET imgui::imgui)
    set(TEXRENDER_IMGUI_TARGET imgui::imgui CACHE INTERNAL "Dear ImGui target used by texrender" FORCE)
elseif (TARGET imgui)
    set(TEXRENDER_IMGUI_TARGET imgui CACHE INTERNAL "Dear ImGui target used by texrender" FORCE)
else ()
    message(FATAL_ERROR "TEXRENDER_BACKEND=imgui requires a Dear ImGui CMake target")
endif ()

# imgui_internal.h and the platform/renderer backends are needed for the glyph atlas and examples.
if (NOT TEXRENDER_IMGUI_SOURCE_DIR)
    get_target_property(_texrender_imgui_includes ${TEXRENDER_IMGUI_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
    foreach (_texrender_imgui_include IN LISTS _texrender_imgui_includes)
        if (EXISTS "${_texrender_imgui_include}/imgui_internal.h")
            set(TEXRENDER_IMGUI_SOURCE_DIR "${_texrender_imgui_include}" CACHE INTERNAL
                    "Dear ImGui source directory used by texrender" FORCE)
            break ()
        endif ()
    endforeach ()
    unset(_texrender_imgui_includes)
    unset(_texrender_imgui_include)
endif ()

if (NOT TEXRENDER_IMGUI_SOURCE_DIR)
    message(FATAL_ERROR
            "TEXRENDER_BACKEND=imgui needs imgui_internal.h, which was not found next to the installed Dear ImGui "
            "headers. Build against the third_party/imgui submodule instead.")
endif ()

set(ImGui_FOUND TRUE)
