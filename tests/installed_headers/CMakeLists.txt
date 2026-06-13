cmake_minimum_required(VERSION 3.21)

project(texgui_installed_headers_smoke LANGUAGES CXX)

find_package(texgui CONFIG REQUIRED)

if(NOT DEFINED TEXGUI_INSTALLED_INCLUDE_DIR)
    message(FATAL_ERROR "TEXGUI_INSTALLED_INCLUDE_DIR must point at the installed include directory")
endif()

get_filename_component(TEXGUI_INSTALLED_INCLUDE_DIR "${TEXGUI_INSTALLED_INCLUDE_DIR}" ABSOLUTE)
if(NOT IS_DIRECTORY "${TEXGUI_INSTALLED_INCLUDE_DIR}/texgui")
    message(FATAL_ERROR "Missing installed texgui include directory: ${TEXGUI_INSTALLED_INCLUDE_DIR}/texgui")
endif()

file(GLOB TEXGUI_INSTALLED_HEADERS CONFIGURE_DEPENDS "${TEXGUI_INSTALLED_INCLUDE_DIR}/texgui/*.hpp")
list(SORT TEXGUI_INSTALLED_HEADERS)
if(NOT TEXGUI_INSTALLED_HEADERS)
    message(FATAL_ERROR "No installed texgui headers found under ${TEXGUI_INSTALLED_INCLUDE_DIR}/texgui")
endif()

set(header_smoke_source "#include <type_traits>\n")
foreach(header IN LISTS TEXGUI_INSTALLED_HEADERS)
    file(RELATIVE_PATH relative_header "${TEXGUI_INSTALLED_INCLUDE_DIR}" "${header}")
    string(APPEND header_smoke_source "#include <${relative_header}>\n")
endforeach()
string(APPEND header_smoke_source
    "\n"
    "int main()\n"
    "{\n"
    "    static_assert(TexGui::version_major == TEXGUI_VERSION_MAJOR);\n"
    "    static_assert(!std::is_copy_constructible_v<TexGui::LatexDocument>);\n"
    "    return 0;\n"
    "}\n"
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/installed_headers.cpp" "${header_smoke_source}")

add_executable(texgui_installed_headers_smoke "${CMAKE_CURRENT_BINARY_DIR}/installed_headers.cpp")
target_link_libraries(texgui_installed_headers_smoke PRIVATE texgui::texgui)
set_target_properties(texgui_installed_headers_smoke PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

enable_testing()
add_test(NAME installed_headers_compile COMMAND texgui_installed_headers_smoke)
