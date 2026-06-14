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
        "backend": "raylib",
        "with_examples": False,
        "with_tests": False,
        "with_benchmarks": False,
    }
    exports_sources = (
        "CMakeLists.txt",
        "LICENSE",
        "cmake/*",
        "include/*",
        "source/*",
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
        helper_path = os.path.join(self.package_folder, "lib", "cmake", "texrender", "texrenderConanHelpers.cmake")
        save(
            self,
            helper_path,
            'set(TEXRENDER_RESOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../share/texrender/res")\n'
            'include("${CMAKE_CURRENT_LIST_DIR}/texrenderHelpers.cmake")\n',
        )

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "texrender")
        self.cpp_info.set_property("cmake_target_name", "texrender::texrender")
        self.cpp_info.set_property(
            "cmake_build_modules",
            [os.path.join("lib", "cmake", "texrender", "texrenderConanHelpers.cmake")],
        )
        self.cpp_info.libs = ["texrender", "microtex_core"]
