if(NOT DEFINED TEXGUI_SOURCE_DIR)
    message(FATAL_ERROR "TEXGUI_SOURCE_DIR is required")
endif()

if(NOT DEFINED TEXGUI_CONSUMER_BUILD_DIR)
    message(FATAL_ERROR "TEXGUI_CONSUMER_BUILD_DIR is required")
endif()

if(NOT DEFINED TEXGUI_CONSUMER_CMAKE_BUILD_TYPE OR TEXGUI_CONSUMER_CMAKE_BUILD_TYPE STREQUAL "")
    set(TEXGUI_CONSUMER_CMAKE_BUILD_TYPE Release)
endif()

file(REMOVE_RECURSE "${TEXGUI_CONSUMER_BUILD_DIR}")

set(configure_command
    "${CMAKE_COMMAND}"
    -S "${CMAKE_CURRENT_LIST_DIR}"
    -B "${TEXGUI_CONSUMER_BUILD_DIR}"
    -DCMAKE_BUILD_TYPE=${TEXGUI_CONSUMER_CMAKE_BUILD_TYPE}
    -DTEXGUI_SOURCE_DIR=${TEXGUI_SOURCE_DIR}
)
if(DEFINED TEXGUI_CONSUMER_GENERATOR AND NOT TEXGUI_CONSUMER_GENERATOR STREQUAL "")
    list(APPEND configure_command -G "${TEXGUI_CONSUMER_GENERATOR}")
endif()
if(DEFINED TEXGUI_CONSUMER_CXX_COMPILER AND NOT TEXGUI_CONSUMER_CXX_COMPILER STREQUAL "")
    list(APPEND configure_command -DCMAKE_CXX_COMPILER=${TEXGUI_CONSUMER_CXX_COMPILER})
endif()

execute_process(COMMAND ${configure_command} RESULT_VARIABLE configure_result)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "consumer smoke configure failed with ${configure_result}")
endif()

set(build_command "${CMAKE_COMMAND}" --build "${TEXGUI_CONSUMER_BUILD_DIR}")
if(DEFINED TEXGUI_TEST_CONFIG AND NOT TEXGUI_TEST_CONFIG STREQUAL "")
    list(APPEND build_command --config "${TEXGUI_TEST_CONFIG}")
endif()
execute_process(COMMAND ${build_command} RESULT_VARIABLE build_result)
if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "consumer smoke build failed with ${build_result}")
endif()

set(test_command "${CMAKE_CTEST_COMMAND}" --test-dir "${TEXGUI_CONSUMER_BUILD_DIR}" --output-on-failure)
if(DEFINED TEXGUI_TEST_CONFIG AND NOT TEXGUI_TEST_CONFIG STREQUAL "")
    list(APPEND test_command -C "${TEXGUI_TEST_CONFIG}")
endif()
execute_process(COMMAND ${test_command} RESULT_VARIABLE test_result)
if(NOT test_result EQUAL 0)
    message(FATAL_ERROR "consumer smoke test failed with ${test_result}")
endif()
