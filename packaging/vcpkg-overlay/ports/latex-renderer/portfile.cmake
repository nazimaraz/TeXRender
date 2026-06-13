if(NOT DEFINED ENV{TEXGUI_VCPKG_SOURCE_DIR})
    message(FATAL_ERROR "TEXGUI_VCPKG_SOURCE_DIR must point at the latex-renderer checkout for this overlay smoke test")
endif()

set(SOURCE_PATH "$ENV{TEXGUI_VCPKG_SOURCE_DIR}")
set(TEXGUI_BACKEND "headless")
if("raylib" IN_LIST FEATURES)
    set(TEXGUI_BACKEND "raylib")
elseif("sdl2" IN_LIST FEATURES)
    set(TEXGUI_BACKEND "sdl2")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DTEXGUI_BACKEND=${TEXGUI_BACKEND}
        -DTEXGUI_BUILD_EXAMPLES=OFF
        -DTEXGUI_BUILD_TESTS=OFF
        -DTEXGUI_BUILD_BENCHMARKS=OFF
        -DTEXGUI_BUILD_FUZZERS=OFF
        -DTEXGUI_INSTALL=ON
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME texgui CONFIG_PATH lib/cmake/texgui)
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
