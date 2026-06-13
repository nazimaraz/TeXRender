cmake_minimum_required(VERSION 3.21)

project(texgui_fetchcontent_example LANGUAGES CXX)

include(FetchContent)

# For local development:
#   cmake -S examples/consumer-fetchcontent -B build-fetch-example \
#     -DTEXGUI_FETCHCONTENT_SOURCE_DIR=/path/to/latex-renderer
set(TEXGUI_FETCHCONTENT_SOURCE_DIR "" CACHE PATH "Optional local texgui checkout")

set(TEXGUI_BACKEND headless CACHE STRING "" FORCE)
set(TEXGUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TEXGUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TEXGUI_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(TEXGUI_BUILD_FUZZERS OFF CACHE BOOL "" FORCE)

if(TEXGUI_FETCHCONTENT_SOURCE_DIR)
    FetchContent_Declare(texgui SOURCE_DIR "${TEXGUI_FETCHCONTENT_SOURCE_DIR}")
else()
    FetchContent_Declare(
        texgui
        GIT_REPOSITORY https://github.com/nazimaraz/latex-renderer.git
        GIT_TAG v0.1.0
    )
endif()

FetchContent_MakeAvailable(texgui)

add_executable(texgui_fetchcontent_example main.cpp)
target_compile_features(texgui_fetchcontent_example PRIVATE cxx_std_17)
target_link_libraries(texgui_fetchcontent_example PRIVATE texgui::texgui)
texgui_copy_resources(texgui_fetchcontent_example)
