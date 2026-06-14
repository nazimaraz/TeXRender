cmake_minimum_required(VERSION 3.21)

project(texrender_backend_package_consumer LANGUAGES CXX)

if(NOT DEFINED TEXRENDER_BACKEND_PACKAGE OR TEXRENDER_BACKEND_PACKAGE STREQUAL "")
    message(FATAL_ERROR "TEXRENDER_BACKEND_PACKAGE must be raylib or sdl2")
endif()
if(NOT TEXRENDER_BACKEND_PACKAGE MATCHES "^(raylib|sdl2)$")
    message(FATAL_ERROR "Unsupported TEXRENDER_BACKEND_PACKAGE='${TEXRENDER_BACKEND_PACKAGE}'")
endif()

find_package(texrender CONFIG REQUIRED)

string(TOUPPER "${TEXRENDER_BACKEND_PACKAGE}" TEXRENDER_BACKEND_PACKAGE_UPPER)

add_executable(texrender_backend_package_consumer main.cpp)
target_link_libraries(texrender_backend_package_consumer PRIVATE texrender::texrender)
target_compile_definitions(texrender_backend_package_consumer PRIVATE
    "TEXRENDER_BACKEND_PACKAGE_${TEXRENDER_BACKEND_PACKAGE_UPPER}=1"
)
set_target_properties(texrender_backend_package_consumer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

enable_testing()
add_test(NAME backend_package_consumer_runs COMMAND texrender_backend_package_consumer)
