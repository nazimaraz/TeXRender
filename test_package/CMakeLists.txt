cmake_minimum_required(VERSION 3.21)

project(texrender_conan_test_package LANGUAGES CXX)

set(TEXRENDER_EXPECT_BACKEND "" CACHE STRING "Expected backend from Conan package option")
set(TEXRENDER_EXPECT_HAS_RAYLIB "" CACHE STRING "Expected raylib feature from Conan package option")
set(TEXRENDER_EXPECT_HAS_SDL2 "" CACHE STRING "Expected SDL2 feature from Conan package option")

find_package(texrender CONFIG REQUIRED)

if(DEFINED texrender_BACKEND AND TEXRENDER_EXPECT_BACKEND AND NOT "${texrender_BACKEND}" STREQUAL "${TEXRENDER_EXPECT_BACKEND}")
    message(FATAL_ERROR "Expected texrender_BACKEND='${TEXRENDER_EXPECT_BACKEND}', got '${texrender_BACKEND}'")
endif()

add_executable(texrender_conan_test_package main.cpp)
target_compile_features(texrender_conan_test_package PRIVATE cxx_std_17)
target_link_libraries(texrender_conan_test_package PRIVATE texrender::texrender)
if(TEXRENDER_EXPECT_BACKEND)
    set(expected_raylib_int 0)
    if(TEXRENDER_EXPECT_HAS_RAYLIB)
        set(expected_raylib_int 1)
    endif()
    set(expected_sdl2_int 0)
    if(TEXRENDER_EXPECT_HAS_SDL2)
        set(expected_sdl2_int 1)
    endif()
    target_compile_definitions(texrender_conan_test_package PRIVATE
        "TEXRENDER_EXPECT_BACKEND=\"${TEXRENDER_EXPECT_BACKEND}\""
        TEXRENDER_EXPECT_HAS_RAYLIB=${expected_raylib_int}
        TEXRENDER_EXPECT_HAS_SDL2=${expected_sdl2_int}
    )
endif()
texrender_copy_resources(texrender_conan_test_package)
