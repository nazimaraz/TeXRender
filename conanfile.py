from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy, save
import os

required_conan_version = ">=2.28"


class TeXRenderConan(ConanFile):
    name = "texrender"
    version = "0.1.0"
    license = "MIT"
    author = "Nazim Can"
    url = "https://github.com/nazimaraz/TeXRender"
    description = "C++ LaTeX math renderer with raylib, SDL2, headless RGBA, and SVG backends"
    topics = ("latex", "renderer", "raylib", "sdl2", "svg", "freetype", "cmake", "webassembly")
    package_type = "library"

    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "backend": ["headless", "raylib", "svg", "sdl2"],
        "with_examples": [True, False],
        "with_tests": [True, False],
        "with_benchmarks": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "backend": "headless",
        "with_examples": False,
        "with_tests": False,
        "with_benchmarks": False,
    }
    exports_sources = (
        "CMakeLists.txt",
        "LICENSE",
        "THIRD_PARTY.md",
        ".gitmodules",
        "cmake/*",
        "include/*",
        "source/*",
        "third_party/microtex/src/*",
        "third_party/microtex/res/*",
        "examples/*",
        "benchmarks/*",
        "tests/*",
        "docs/*",
    )

    def requirements(self):
        self.requires("freetype/2.13.2")
        self.requires("tinyxml2/10.0.0")
        if self.options.backend == "raylib":
            self.requires("raylib/5.5")
        elif self.options.backend == "sdl2":
            self.requires("sdl/[~2.32]")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self)
        tc.variables["BUILD_SHARED_LIBS"] = bool(self.options.shared)
        tc.variables["TEXRENDER_BACKEND"] = str(self.options.backend)
        tc.variables["TEXRENDER_BUILD_EXAMPLES"] = bool(self.options.with_examples)
        tc.variables["TEXRENDER_BUILD_TESTS"] = bool(self.options.with_tests)
        tc.variables["TEXRENDER_BUILD_BENCHMARKS"] = bool(self.options.with_benchmarks)
        tc.variables["TEXRENDER_BUILD_FUZZERS"] = False
        tc.variables["TEXRENDER_INSTALL"] = True
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, "LICENSE", self.source_folder, os.path.join(self.package_folder, "licenses"))
        copy(self, "THIRD_PARTY.md", self.source_folder, os.path.join(self.package_folder, "licenses"))
        helper_path = os.path.join(self.package_folder, "lib", "cmake", "texrender", "texrenderConanHelpers.cmake")
        save(
            self,
            helper_path,
            'set(TEXRENDER_RESOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../share/texrender/res")\n'
            'include("${CMAKE_CURRENT_LIST_DIR}/texrenderHelpers.cmake")\n'
            + self._conan_backend_helper(),
        )

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "texrender")
        self.cpp_info.set_property("cmake_target_name", "texrender::texrender")
        self.cpp_info.set_property(
            "cmake_build_modules",
            [os.path.join("lib", "cmake", "texrender", "texrenderConanHelpers.cmake")],
        )
        self.cpp_info.libs = ["texrender", "microtex_core"]
        self.cpp_info.requires = ["freetype::freetype", "tinyxml2::tinyxml2"]
        if self.options.backend == "raylib":
            self.cpp_info.requires.append("raylib::raylib")
        elif self.options.backend == "sdl2":
            self.cpp_info.requires.append("sdl::libsdl2")

    def _conan_backend_helper(self):
        backend = str(self.options.backend)
        if backend == "raylib":
            return (
                'if(TARGET texrender::texrender AND TARGET raylib)\n'
                '  target_link_libraries(texrender::texrender INTERFACE raylib)\n'
                '  set(_texrender_raylib_includes ${raylib_INCLUDE_DIRS})\n'
                '  foreach(_texrender_config RELEASE DEBUG RELWITHDEBINFO MINSIZEREL)\n'
                '    set(_texrender_raylib_package_var "raylib_PACKAGE_FOLDER_${_texrender_config}")\n'
                '    if(DEFINED ${_texrender_raylib_package_var})\n'
                '      list(APPEND _texrender_raylib_includes "${${_texrender_raylib_package_var}}/include")\n'
                '    endif()\n'
                '  endforeach()\n'
                '  foreach(_texrender_include_dir IN LISTS CMAKE_INCLUDE_PATH)\n'
                '    if(EXISTS "${_texrender_include_dir}/raylib.h")\n'
                '      list(APPEND _texrender_raylib_includes "${_texrender_include_dir}")\n'
                '    endif()\n'
                '  endforeach()\n'
                '  unset(_texrender_include_dir)\n'
                '  unset(_texrender_raylib_package_var)\n'
                '  if(_texrender_raylib_includes)\n'
                '    list(REMOVE_DUPLICATES _texrender_raylib_includes)\n'
                '    target_include_directories(texrender::texrender INTERFACE ${_texrender_raylib_includes})\n'
                '  endif()\n'
                '  unset(_texrender_config)\n'
                '  unset(_texrender_raylib_includes)\n'
                'endif()\n'
            )
        if backend == "sdl2":
            return (
                'if(TARGET texrender::texrender)\n'
                '  set(_texrender_sdl2_targets SDL2::SDL2 SDL2::SDL2-static SDL2)\n'
                '  foreach(_texrender_sdl2_target IN LISTS _texrender_sdl2_targets)\n'
                '    if(TARGET ${_texrender_sdl2_target})\n'
                '      target_link_libraries(texrender::texrender INTERFACE ${_texrender_sdl2_target})\n'
                '      set(_texrender_sdl2_includes ${sdl_INCLUDE_DIRS})\n'
                '      foreach(_texrender_config RELEASE DEBUG RELWITHDEBINFO MINSIZEREL)\n'
                '        set(_texrender_sdl2_package_var "sdl_PACKAGE_FOLDER_${_texrender_config}")\n'
                '        if(DEFINED ${_texrender_sdl2_package_var})\n'
                '          list(APPEND _texrender_sdl2_includes "${${_texrender_sdl2_package_var}}/include")\n'
                '        endif()\n'
                '      endforeach()\n'
                '      foreach(_texrender_include_dir IN LISTS CMAKE_INCLUDE_PATH)\n'
                '        if(EXISTS "${_texrender_include_dir}/SDL.h" OR EXISTS "${_texrender_include_dir}/SDL2/SDL.h")\n'
                '          list(APPEND _texrender_sdl2_includes "${_texrender_include_dir}")\n'
                '        endif()\n'
                '      endforeach()\n'
                '      unset(_texrender_include_dir)\n'
                '      unset(_texrender_sdl2_package_var)\n'
                '      if(_texrender_sdl2_includes)\n'
                '        list(REMOVE_DUPLICATES _texrender_sdl2_includes)\n'
                '        target_include_directories(texrender::texrender INTERFACE ${_texrender_sdl2_includes})\n'
                '      endif()\n'
                '      unset(_texrender_config)\n'
                '      unset(_texrender_sdl2_includes)\n'
                '      break()\n'
                '    endif()\n'
                '  endforeach()\n'
                '  unset(_texrender_sdl2_targets)\n'
                'endif()\n'
            )
        return ""
