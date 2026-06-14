if(NOT DEFINED TEXRENDER_SOURCE_DIR)
    message(FATAL_ERROR "TEXRENDER_SOURCE_DIR is required")
endif()

if(NOT DEFINED TEXRENDER_CONSUMER_BUILD_DIR)
    message(FATAL_ERROR "TEXRENDER_CONSUMER_BUILD_DIR is required")
endif()

if(NOT DEFINED TEXRENDER_CONSUMER_CMAKE_BUILD_TYPE OR TEXRENDER_CONSUMER_CMAKE_BUILD_TYPE STREQUAL "")
    set(TEXRENDER_CONSUMER_CMAKE_BUILD_TYPE Release)
endif()

file(REMOVE_RECURSE "${TEXRENDER_CONSUMER_BUILD_DIR}")

set(configure_command
    "${CMAKE_COMMAND}"
    -S "${CMAKE_CURRENT_LIST_DIR}"
    -B "${TEXRENDER_CONSUMER_BUILD_DIR}"
    -DCMAKE_BUILD_TYPE=${TEXRENDER_CONSUMER_CMAKE_BUILD_TYPE}
    -DTEXRENDER_SOURCE_DIR=${TEXRENDER_SOURCE_DIR}
)
if(DEFINED TEXRENDER_CONSUMER_GENERATOR AND NOT TEXRENDER_CONSUMER_GENERATOR STREQUAL "")
    list(APPEND configure_command -G "${TEXRENDER_CONSUMER_GENERATOR}")
endif()
if(DEFINED TEXRENDER_CONSUMER_CXX_COMPILER AND NOT TEXRENDER_CONSUMER_CXX_COMPILER STREQUAL "")
    list(APPEND configure_command -DCMAKE_CXX_COMPILER=${TEXRENDER_CONSUMER_CXX_COMPILER})
endif()

execute_process(COMMAND ${configure_command} RESULT_VARIABLE configure_result)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "consumer smoke configure failed with ${configure_result}")
endif()

set(build_command "${CMAKE_COMMAND}" --build "${TEXRENDER_CONSUMER_BUILD_DIR}")
if(DEFINED TEXRENDER_TEST_CONFIG AND NOT TEXRENDER_TEST_CONFIG STREQUAL "")
    list(APPEND build_command --config "${TEXRENDER_TEST_CONFIG}")
endif()
execute_process(COMMAND ${build_command} RESULT_VARIABLE build_result)
if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "consumer smoke build failed with ${build_result}")
endif()

set(test_command "${CMAKE_CTEST_COMMAND}" --test-dir "${TEXRENDER_CONSUMER_BUILD_DIR}" --output-on-failure)
if(DEFINED TEXRENDER_TEST_CONFIG AND NOT TEXRENDER_TEST_CONFIG STREQUAL "")
    list(APPEND test_command -C "${TEXRENDER_TEST_CONFIG}")
endif()
execute_process(COMMAND ${test_command} RESULT_VARIABLE test_result)
if(NOT test_result EQUAL 0)
    message(FATAL_ERROR "consumer smoke test failed with ${test_result}")
endif()
