cmake_minimum_required(VERSION 3.21)

project(texgui_package_consumer_smoke LANGUAGES CXX)

find_package(texgui CONFIG REQUIRED)

add_executable(texgui_package_consumer ../consumer/main.cpp)
target_link_libraries(texgui_package_consumer PRIVATE texgui::texgui)
set_target_properties(texgui_package_consumer PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
texgui_copy_resources(texgui_package_consumer)
if(WIN32)
    add_custom_command(TARGET texgui_package_consumer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:texgui_package_consumer>
                $<TARGET_FILE_DIR:texgui_package_consumer>
        COMMAND_EXPAND_LISTS
    )
endif()

enable_testing()
add_test(NAME package_consumer_runs COMMAND texgui_package_consumer)
