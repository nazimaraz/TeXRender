cmake_minimum_required(VERSION 3.21)

project(texgui_backend_package_consumer LANGUAGES CXX)

if(NOT DEFINED TEXGUI_BACKEND_PACKAGE OR TEXGUI_BACKEND_PACKAGE STREQUAL "")
    message(FATAL_ERROR "TEXGUI_BACKEND_PACKAGE must be raylib or sdl2")
endif()
if(NOT TEXGUI_BACKEND_PACKAGE MATCHES "^(raylib|sdl2)$")
    message(FATAL_ERROR "Unsupported TEXGUI_BACKEND_PACKAGE='${TEXGUI_BACKEND_PACKAGE}'")
endif()

find_package(texgui CONFIG REQUIRED)

string(TOUPPER "${TEXGUI_BACKEND_PACKAGE}" TEXGUI_BACKEND_PACKAGE_UPPER)

add_executable(texgui_backend_package_consumer main.cpp)
target_link_libraries(texgui_backend_package_consumer PRIVATE texgui::texgui)
target_compile_definitions(texgui_backend_package_consumer PRIVATE
    "TEXGUI_BACKEND_PACKAGE_${TEXGUI_BACKEND_PACKAGE_UPPER}=1"
)
set_target_properties(texgui_backend_package_consumer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

enable_testing()
add_test(NAME backend_package_consumer_runs COMMAND texgui_backend_package_consumer)
