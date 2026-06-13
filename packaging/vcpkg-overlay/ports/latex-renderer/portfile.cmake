if(NOT DEFINED ENV{TEXGUI_VCPKG_SOURCE_DIR})
    message(FATAL_ERROR "TEXGUI_VCPKG_SOURCE_DIR must point at the latex-renderer checkout for this overlay smoke test")
endif()

set(SOURCE_PATH "$ENV{TEXGUI_VCPKG_SOURCE_DIR}")
vcpkg_from_github(
    OUT_SOURCE_PATH MICROTEX_SOURCE_PATH
    REPO NanoMichael/MicroTeX
    REF 0e3707f6dafebb121d98b53c64364d16fefe481d
    SHA512 7f3c0d4218aaee91b7fb1b402e545228c024a9d07479aafbda1f50daf1b37761e57887c89a356eb5ff2073b0a77b528758208540ccf55ef4922a370e5335a400
    HEAD_REF main
)

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
        -DFETCHCONTENT_SOURCE_DIR_MICROTEX=${MICROTEX_SOURCE_PATH}
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME texgui CONFIG_PATH lib/cmake/texgui)
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
