# Package Publishing

The repository includes starter metadata for Conan and vcpkg. Keep these paths
green before submitting to public registries.

## Conan

Local package validation:

```sh
conan profile detect --force
conan create . --build=missing \
  -o "&:backend=headless" \
  -o "&:with_examples=False" \
  -o "&:with_tests=False" \
  -o "&:with_benchmarks=False"
conan create . --build=missing \
  -o "&:backend=svg" \
  -o "&:with_examples=False" \
  -o "&:with_tests=False" \
  -o "&:with_benchmarks=False"
conan create . --build=missing \
  -o "&:backend=sdl2" \
  -o "&:with_examples=False" \
  -o "&:with_tests=False" \
  -o "&:with_benchmarks=False"
```

The `test_package/` consumer exercises public headers, cache/layout helpers,
headless rendering, SVG export, and runtime resource copying. Add coverage there
when package-facing API changes.

Before a Conan Center submission:

- confirm `license`, `url`, `description`, `topics`, options, and default
  options are current in `conanfile.py`
- make sure the recipe exports only source files needed to build/install
- check whether dependency versions should be relaxed or updated
- run both dependency-free backend options (`headless`, `svg`)
- document raylib and SDL2 backend expectations separately because they carry
  additional GUI dependencies

## vcpkg

Project-local manifest validation:

```sh
cmake -S . -B build-vcpkg \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON \
  -DTEXGUI_BACKEND=headless \
  -DTEXGUI_BUILD_EXAMPLES=OFF \
  -DTEXGUI_BUILD_BENCHMARKS=OFF
cmake --build build-vcpkg -j
ctest --test-dir build-vcpkg --output-on-failure
```

Overlay port smoke test:

```sh
VCPKG_ROOT=/path/to/vcpkg scripts/verify-vcpkg-overlay.sh
```

The overlay port under `packaging/vcpkg-overlay/ports/latex-renderer/` builds
from the current checkout through vcpkg's portfile flow. It is intentionally
local-development shaped: set `TEXGUI_VCPKG_SOURCE_DIR` or run the script, which
sets it for you.

Before a vcpkg registry submission:

- replace the local source path with a release archive or GitHub release ref
- fill in the registry-required source hash
- verify the installed layout follows vcpkg conventions
- test default, `raylib`, and `sdl2` feature builds

## Release Routine

Run `scripts/verify-release.sh` before tagging. It validates CMake builds,
install/export consumers, generated gallery artifacts, package-manager entry
points when local tools are available, release archive smoke, and API
documentation when Doxygen is installed.
