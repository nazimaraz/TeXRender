function(texgui_copy_resources target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "texgui_copy_resources: '${target}' is not a CMake target")
    endif()

    if(NOT DEFINED TEXGUI_RESOURCE_DIR OR TEXGUI_RESOURCE_DIR STREQUAL "")
        message(FATAL_ERROR "texgui_copy_resources: TEXGUI_RESOURCE_DIR is not set")
    endif()

    target_compile_definitions("${target}" PRIVATE "TEXGUI_RES_DIR=\"$<TARGET_FILE_DIR:${target}>/res\"")
    add_custom_command(TARGET "${target}" POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${TEXGUI_RESOURCE_DIR}" "$<TARGET_FILE_DIR:${target}>/res"
        COMMENT "Copying texgui MicroTeX resources for ${target}"
        VERBATIM)
endfunction()
