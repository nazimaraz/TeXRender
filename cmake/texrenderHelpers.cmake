function(texrender_copy_resources target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "texrender_copy_resources: '${target}' is not a CMake target")
    endif()

    if(NOT DEFINED TEXRENDER_RESOURCE_DIR OR TEXRENDER_RESOURCE_DIR STREQUAL "")
        message(FATAL_ERROR "texrender_copy_resources: TEXRENDER_RESOURCE_DIR is not set")
    endif()

    set(resource_output "$<TARGET_FILE_DIR:${target}>/texrender-res/${target}")

    target_compile_definitions("${target}" PRIVATE "TEXRENDER_RES_DIR=\"${resource_output}\"")
    add_custom_command(TARGET "${target}" POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${TEXRENDER_RESOURCE_DIR}" "${resource_output}"
        COMMENT "Copying texrender MicroTeX resources for ${target}"
        VERBATIM)
endfunction()
