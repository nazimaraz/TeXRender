function(texrender_add_submodule_dependency name upper submodule_dir marker)
    if (NOT TEXRENDER_USE_SUBMODULE_DEPENDENCIES)
        message(FATAL_ERROR
                "${name} was not found as an installed package. "
                "Install ${name}, or enable TEXRENDER_USE_SUBMODULE_DEPENDENCIES.")
    endif ()

    if (NOT EXISTS "${submodule_dir}/${marker}")
        message(FATAL_ERROR
                "${name} was not found as an installed package and no usable source checkout exists at "
                "'${submodule_dir}'. Initialize submodules with `git submodule update --init --recursive`.")
    endif ()

    add_subdirectory("${submodule_dir}" "${CMAKE_CURRENT_BINARY_DIR}/_deps/${name}-build" EXCLUDE_FROM_ALL)
    set(TEXRENDER_${upper}_SOURCE_KIND "submodule" CACHE INTERNAL "How texrender resolved ${name}" FORCE)
endfunction()
