cmake_minimum_required(VERSION 3.21)

project(texgui_conan_test_package LANGUAGES CXX)

find_package(texgui CONFIG REQUIRED)

add_executable(texgui_conan_test_package main.cpp)
target_compile_features(texgui_conan_test_package PRIVATE cxx_std_17)
target_link_libraries(texgui_conan_test_package PRIVATE texgui::texgui)
texgui_copy_resources(texgui_conan_test_package)
