cmake_minimum_required(VERSION 3.21)

project(texrender_fetchcontent_example LANGUAGES CXX)

include(FetchContent)

# For local development:
#   cmake -S examples/consumer-fetchcontent -B build-fetch-example \
#     -DTEXRENDER_FETCHCONTENT_SOURCE_DIR=/path/to/TeXRender
set(TEXRENDER_FETCHCONTENT_SOURCE_DIR "" CACHE PATH "Optional local texrender checkout")

set(TEXRENDER_BACKEND headless CACHE STRING "" FORCE)
set(TEXRENDER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TEXRENDER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TEXRENDER_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(TEXRENDER_BUILD_FUZZERS OFF CACHE BOOL "" FORCE)

if(TEXRENDER_FETCHCONTENT_SOURCE_DIR)
    FetchContent_Declare(texrender SOURCE_DIR "${TEXRENDER_FETCHCONTENT_SOURCE_DIR}")
else()
    FetchContent_Declare(
        texrender
        GIT_REPOSITORY https://github.com/nazimaraz/TeXRender.git
        GIT_TAG v0.1.0
    )
endif()

FetchContent_MakeAvailable(texrender)

add_executable(texrender_fetchcontent_example main.cpp)
target_compile_features(texrender_fetchcontent_example PRIVATE cxx_std_17)
target_link_libraries(texrender_fetchcontent_example PRIVATE texrender::texrender)
texrender_copy_resources(texrender_fetchcontent_example)
