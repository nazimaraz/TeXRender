import os

from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout


class TexguiTestPackageConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    test_type = "explicit"

    def requirements(self):
        self.requires(self.tested_reference_str)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        dependency = self.dependencies[self.tested_reference_str]
        backend = str(dependency.options.backend)
        tc = CMakeToolchain(self)
        tc.variables["TEXRENDER_EXPECT_BACKEND"] = backend
        tc.variables["TEXRENDER_EXPECT_HAS_RAYLIB"] = backend == "raylib"
        tc.variables["TEXRENDER_EXPECT_HAS_SDL2"] = backend == "sdl2"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if can_run(self):
            cmd = os.path.join(self.cpp.build.bindirs[0], "texrender_conan_test_package")
            self.run(cmd, env="conanrun")
