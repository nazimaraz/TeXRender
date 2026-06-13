cmake_minimum_required(VERSION 3.21)

project(texgui_conan_test_package LANGUAGES CXX)

set(TEXGUI_EXPECT_BACKEND "" CACHE STRING "Expected backend from Conan package option")
set(TEXGUI_EXPECT_HAS_RAYLIB "" CACHE STRING "Expected raylib feature from Conan package option")
set(TEXGUI_EXPECT_HAS_SDL2 "" CACHE STRING "Expected SDL2 feature from Conan package option")

find_package(texgui CONFIG REQUIRED)

if(DEFINED texgui_BACKEND AND TEXGUI_EXPECT_BACKEND AND NOT "${texgui_BACKEND}" STREQUAL "${TEXGUI_EXPECT_BACKEND}")
    message(FATAL_ERROR "Expected texgui_BACKEND='${TEXGUI_EXPECT_BACKEND}', got '${texgui_BACKEND}'")
endif()

add_executable(texgui_conan_test_package main.cpp)
target_compile_features(texgui_conan_test_package PRIVATE cxx_std_17)
target_link_libraries(texgui_conan_test_package PRIVATE texgui::texgui)
if(TEXGUI_EXPECT_BACKEND)
    set(expected_raylib_int 0)
    if(TEXGUI_EXPECT_HAS_RAYLIB)
        set(expected_raylib_int 1)
    endif()
    set(expected_sdl2_int 0)
    if(TEXGUI_EXPECT_HAS_SDL2)
        set(expected_sdl2_int 1)
    endif()
    target_compile_definitions(texgui_conan_test_package PRIVATE
        "TEXGUI_EXPECT_BACKEND=\"${TEXGUI_EXPECT_BACKEND}\""
        TEXGUI_EXPECT_HAS_RAYLIB=${expected_raylib_int}
        TEXGUI_EXPECT_HAS_SDL2=${expected_sdl2_int}
    )
endif()
texgui_copy_resources(texgui_conan_test_package)
