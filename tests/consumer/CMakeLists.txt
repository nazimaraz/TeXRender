cmake_minimum_required(VERSION 3.21)

project(texgui_consumer_smoke LANGUAGES CXX)

if(NOT DEFINED TEXGUI_SOURCE_DIR)
    message(FATAL_ERROR "TEXGUI_SOURCE_DIR must point at the latex-renderer source tree")
endif()

set(TEXGUI_BACKEND headless CACHE STRING "" FORCE)
set(TEXGUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TEXGUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory("${TEXGUI_SOURCE_DIR}" texgui-build)

add_executable(texgui_consumer main.cpp)
target_link_libraries(texgui_consumer PRIVATE texgui::texgui)
set_target_properties(texgui_consumer PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
texgui_copy_resources(texgui_consumer)

enable_testing()
add_test(NAME consumer_runs COMMAND texgui_consumer)
