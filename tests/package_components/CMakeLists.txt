cmake_minimum_required(VERSION 3.21)

project(texgui_package_components_smoke LANGUAGES CXX)

set(TEXGUI_REQUIRED_COMPONENTS "" CACHE STRING "texgui components to request")
set(TEXGUI_EXPECT_BACKEND "" CACHE STRING "Expected texgui_BACKEND value")
set(TEXGUI_EXPECT_HAS_RAYLIB "" CACHE STRING "Expected texgui_HAS_RAYLIB value")
set(TEXGUI_EXPECT_HAS_SDL2 "" CACHE STRING "Expected texgui_HAS_SDL2 value")

if(TEXGUI_REQUIRED_COMPONENTS)
    find_package(texgui CONFIG REQUIRED COMPONENTS ${TEXGUI_REQUIRED_COMPONENTS})
else()
    find_package(texgui CONFIG REQUIRED)
endif()

if(TEXGUI_EXPECT_BACKEND STREQUAL "")
    message(FATAL_ERROR "TEXGUI_EXPECT_BACKEND is required")
endif()
if(TEXGUI_EXPECT_HAS_RAYLIB STREQUAL "" OR TEXGUI_EXPECT_HAS_SDL2 STREQUAL "")
    message(FATAL_ERROR "TEXGUI_EXPECT_HAS_RAYLIB and TEXGUI_EXPECT_HAS_SDL2 are required")
endif()

if(NOT "${texgui_BACKEND}" STREQUAL "${TEXGUI_EXPECT_BACKEND}")
    message(FATAL_ERROR "Expected texgui_BACKEND='${TEXGUI_EXPECT_BACKEND}', got '${texgui_BACKEND}'")
endif()
if(NOT "${texgui_HAS_HEADLESS}" STREQUAL "TRUE")
    message(FATAL_ERROR "Expected texgui_HAS_HEADLESS=TRUE, got '${texgui_HAS_HEADLESS}'")
endif()
if(NOT "${texgui_HAS_SVG}" STREQUAL "TRUE")
    message(FATAL_ERROR "Expected texgui_HAS_SVG=TRUE, got '${texgui_HAS_SVG}'")
endif()
if(NOT "${texgui_HAS_RAYLIB}" STREQUAL "${TEXGUI_EXPECT_HAS_RAYLIB}")
    message(FATAL_ERROR "Expected texgui_HAS_RAYLIB='${TEXGUI_EXPECT_HAS_RAYLIB}', got '${texgui_HAS_RAYLIB}'")
endif()
if(NOT "${texgui_HAS_SDL2}" STREQUAL "${TEXGUI_EXPECT_HAS_SDL2}")
    message(FATAL_ERROR "Expected texgui_HAS_SDL2='${TEXGUI_EXPECT_HAS_SDL2}', got '${texgui_HAS_SDL2}'")
endif()

set(expected_raylib_int 0)
if(TEXGUI_EXPECT_HAS_RAYLIB)
    set(expected_raylib_int 1)
endif()
set(expected_sdl2_int 0)
if(TEXGUI_EXPECT_HAS_SDL2)
    set(expected_sdl2_int 1)
endif()

add_executable(texgui_package_components main.cpp)
target_link_libraries(texgui_package_components PRIVATE texgui::texgui)
target_compile_definitions(texgui_package_components PRIVATE
    "TEXGUI_EXPECT_BACKEND=\"${TEXGUI_EXPECT_BACKEND}\""
    TEXGUI_EXPECT_HAS_RAYLIB=${expected_raylib_int}
    TEXGUI_EXPECT_HAS_SDL2=${expected_sdl2_int}
)
set_target_properties(texgui_package_components PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

enable_testing()
add_test(NAME package_components_run COMMAND texgui_package_components)
