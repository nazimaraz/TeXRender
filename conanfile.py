from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy
import os


class LatexRendererConan(ConanFile):
    name = "latex-renderer"
    version = "0.1.0"
    license = "MIT"
    author = "Nazim Can"
    url = "https://github.com/nazimaraz/latex-renderer"
    description = "C++ LaTeX math renderer with raylib, headless RGBA, and SVG backends"
    topics = ("latex", "renderer", "raylib", "svg", "freetype", "cmake", "webassembly")
    package_type = "library"

    settings = "os", "compiler", "build_type", "arch"
    options = {
        "backend": ["headless", "raylib", "svg"],
        "with_examples": [True, False],
        "with_tests": [True, False],
        "with_benchmarks": [True, False],
    }
    default_options = {
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

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self)
        tc.variables["TEXGUI_BACKEND"] = str(self.options.backend)
        tc.variables["TEXGUI_BUILD_EXAMPLES"] = bool(self.options.with_examples)
        tc.variables["TEXGUI_BUILD_TESTS"] = bool(self.options.with_tests)
        tc.variables["TEXGUI_BUILD_BENCHMARKS"] = bool(self.options.with_benchmarks)
        tc.variables["TEXGUI_BUILD_FUZZERS"] = False
        tc.variables["TEXGUI_INSTALL"] = True
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, "LICENSE", self.source_folder, os.path.join(self.package_folder, "licenses"))

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "texgui")
        self.cpp_info.set_property("cmake_target_name", "texgui::texgui")
        self.cpp_info.libs = ["texgui", "microtex_core"]
