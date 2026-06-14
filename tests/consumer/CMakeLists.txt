cmake_minimum_required(VERSION 3.21)

project(texrender_consumer_smoke LANGUAGES CXX)

if(NOT DEFINED TEXRENDER_SOURCE_DIR)
    message(FATAL_ERROR "TEXRENDER_SOURCE_DIR must point at the TeXRender source tree")
endif()

set(TEXRENDER_BACKEND headless CACHE STRING "" FORCE)
set(TEXRENDER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TEXRENDER_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory("${TEXRENDER_SOURCE_DIR}" texrender-build)

add_executable(texrender_consumer main.cpp)
target_link_libraries(texrender_consumer PRIVATE texrender::texrender)
set_target_properties(texrender_consumer PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
texrender_copy_resources(texrender_consumer)

enable_testing()
add_test(NAME consumer_runs COMMAND texrender_consumer)
