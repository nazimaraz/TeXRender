cmake_minimum_required(VERSION 3.21)

project(texrender_package_components_smoke LANGUAGES CXX)

set(TEXRENDER_REQUIRED_COMPONENTS "" CACHE STRING "texrender components to request")
set(TEXRENDER_EXPECT_BACKEND "" CACHE STRING "Expected texrender_BACKEND value")
set(TEXRENDER_EXPECT_HAS_RAYLIB "" CACHE STRING "Expected texrender_HAS_RAYLIB value")
set(TEXRENDER_EXPECT_HAS_SDL2 "" CACHE STRING "Expected texrender_HAS_SDL2 value")

if(TEXRENDER_REQUIRED_COMPONENTS)
    find_package(texrender CONFIG REQUIRED COMPONENTS ${TEXRENDER_REQUIRED_COMPONENTS})
else()
    find_package(texrender CONFIG REQUIRED)
endif()

if(TEXRENDER_EXPECT_BACKEND STREQUAL "")
    message(FATAL_ERROR "TEXRENDER_EXPECT_BACKEND is required")
endif()
if(TEXRENDER_EXPECT_HAS_RAYLIB STREQUAL "" OR TEXRENDER_EXPECT_HAS_SDL2 STREQUAL "")
    message(FATAL_ERROR "TEXRENDER_EXPECT_HAS_RAYLIB and TEXRENDER_EXPECT_HAS_SDL2 are required")
endif()

if(NOT "${texrender_BACKEND}" STREQUAL "${TEXRENDER_EXPECT_BACKEND}")
    message(FATAL_ERROR "Expected texrender_BACKEND='${TEXRENDER_EXPECT_BACKEND}', got '${texrender_BACKEND}'")
endif()
if(NOT "${texrender_HAS_HEADLESS}" STREQUAL "TRUE")
    message(FATAL_ERROR "Expected texrender_HAS_HEADLESS=TRUE, got '${texrender_HAS_HEADLESS}'")
endif()
if(NOT "${texrender_HAS_SVG}" STREQUAL "TRUE")
    message(FATAL_ERROR "Expected texrender_HAS_SVG=TRUE, got '${texrender_HAS_SVG}'")
endif()
if(NOT "${texrender_HAS_RAYLIB}" STREQUAL "${TEXRENDER_EXPECT_HAS_RAYLIB}")
    message(FATAL_ERROR "Expected texrender_HAS_RAYLIB='${TEXRENDER_EXPECT_HAS_RAYLIB}', got '${texrender_HAS_RAYLIB}'")
endif()
if(NOT "${texrender_HAS_SDL2}" STREQUAL "${TEXRENDER_EXPECT_HAS_SDL2}")
    message(FATAL_ERROR "Expected texrender_HAS_SDL2='${TEXRENDER_EXPECT_HAS_SDL2}', got '${texrender_HAS_SDL2}'")
endif()

set(expected_raylib_int 0)
if(TEXRENDER_EXPECT_HAS_RAYLIB)
    set(expected_raylib_int 1)
endif()
set(expected_sdl2_int 0)
if(TEXRENDER_EXPECT_HAS_SDL2)
    set(expected_sdl2_int 1)
endif()

add_executable(texrender_package_components main.cpp)
target_link_libraries(texrender_package_components PRIVATE texrender::texrender)
target_compile_definitions(texrender_package_components PRIVATE
    "TEXRENDER_EXPECT_BACKEND=\"${TEXRENDER_EXPECT_BACKEND}\""
    TEXRENDER_EXPECT_HAS_RAYLIB=${expected_raylib_int}
    TEXRENDER_EXPECT_HAS_SDL2=${expected_sdl2_int}
)
set_target_properties(texrender_package_components PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

enable_testing()
add_test(NAME package_components_run COMMAND texrender_package_components)
