cmake_minimum_required(VERSION 3.21)

project(texrender_installed_headers_smoke LANGUAGES CXX)

find_package(texrender CONFIG REQUIRED)

if(NOT DEFINED TEXRENDER_INSTALLED_INCLUDE_DIR)
    message(FATAL_ERROR "TEXRENDER_INSTALLED_INCLUDE_DIR must point at the installed include directory")
endif()

get_filename_component(TEXRENDER_INSTALLED_INCLUDE_DIR "${TEXRENDER_INSTALLED_INCLUDE_DIR}" ABSOLUTE)
if(NOT IS_DIRECTORY "${TEXRENDER_INSTALLED_INCLUDE_DIR}/texrender")
    message(FATAL_ERROR "Missing installed texrender include directory: ${TEXRENDER_INSTALLED_INCLUDE_DIR}/texrender")
endif()

file(GLOB TEXRENDER_INSTALLED_HEADERS CONFIGURE_DEPENDS "${TEXRENDER_INSTALLED_INCLUDE_DIR}/texrender/*.hpp")
list(SORT TEXRENDER_INSTALLED_HEADERS)
if(NOT TEXRENDER_INSTALLED_HEADERS)
    message(FATAL_ERROR "No installed texrender headers found under ${TEXRENDER_INSTALLED_INCLUDE_DIR}/texrender")
endif()

set(header_smoke_source "#include <type_traits>\n")
foreach(header IN LISTS TEXRENDER_INSTALLED_HEADERS)
    file(RELATIVE_PATH relative_header "${TEXRENDER_INSTALLED_INCLUDE_DIR}" "${header}")
    string(APPEND header_smoke_source "#include <${relative_header}>\n")
endforeach()
string(APPEND header_smoke_source
    "\n"
    "int main()\n"
    "{\n"
    "    static_assert(TeXRender::version_major == TEXRENDER_VERSION_MAJOR);\n"
    "    static_assert(TEXRENDER_HAS_HEADLESS == 1);\n"
    "    static_assert(TEXRENDER_HAS_SVG == 1);\n"
    "    static_assert(TeXRender::has_headless);\n"
    "    static_assert(TeXRender::has_svg);\n"
    "    static_assert(!std::is_copy_constructible_v<TeXRender::LatexDocument>);\n"
    "    return 0;\n"
    "}\n"
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/installed_headers.cpp" "${header_smoke_source}")

add_executable(texrender_installed_headers_smoke "${CMAKE_CURRENT_BINARY_DIR}/installed_headers.cpp")
target_link_libraries(texrender_installed_headers_smoke PRIVATE texrender::texrender)
set_target_properties(texrender_installed_headers_smoke PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

enable_testing()
add_test(NAME installed_headers_compile COMMAND texrender_installed_headers_smoke)

foreach(header IN LISTS TEXRENDER_INSTALLED_HEADERS)
    file(RELATIVE_PATH relative_header "${TEXRENDER_INSTALLED_INCLUDE_DIR}" "${header}")
    get_filename_component(header_filename "${header}" NAME_WE)
    string(MAKE_C_IDENTIFIER "${header_filename}" header_target_suffix)
    string(REPLACE "_" "-" header_test_suffix "${header_target_suffix}")
    set(header_source "${CMAKE_CURRENT_BINARY_DIR}/self-contained/${header_target_suffix}.cpp")
    file(WRITE "${header_source}" "#include <${relative_header}>\n\nint main()\n{\n    return 0;\n}\n")

    add_executable("texrender_installed_header_${header_target_suffix}_self_contained" "${header_source}")
    target_link_libraries("texrender_installed_header_${header_target_suffix}_self_contained" PRIVATE texrender::texrender)
    set_target_properties("texrender_installed_header_${header_target_suffix}_self_contained" PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    add_test(
        NAME "installed_header_self_contained.${header_test_suffix}"
        COMMAND "texrender_installed_header_${header_target_suffix}_self_contained"
    )
endforeach()
