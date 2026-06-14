cmake_minimum_required(VERSION 3.21)

project(texrender_package_consumer_smoke LANGUAGES CXX)

find_package(texrender CONFIG REQUIRED)

add_executable(texrender_package_consumer ../consumer/main.cpp)
target_link_libraries(texrender_package_consumer PRIVATE texrender::texrender)
set_target_properties(texrender_package_consumer PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
texrender_copy_resources(texrender_package_consumer)
if(WIN32)
    add_custom_command(TARGET texrender_package_consumer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:texrender_package_consumer>
                $<TARGET_FILE_DIR:texrender_package_consumer>
        COMMAND_EXPAND_LISTS
    )
endif()

enable_testing()
add_test(NAME package_consumer_runs COMMAND texrender_package_consumer)
