cmake_minimum_required(VERSION 3.21)

project(texgui_find_package_example LANGUAGES CXX)

find_package(texgui CONFIG REQUIRED)

add_executable(texgui_find_package_example main.cpp)
target_compile_features(texgui_find_package_example PRIVATE cxx_std_17)
target_link_libraries(texgui_find_package_example PRIVATE texgui::texgui)
texgui_copy_resources(texgui_find_package_example)
