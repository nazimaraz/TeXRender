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
conan create . --build=missing \
  -o "&:backend=headless" \
  -o "&:shared=True" \
  -o "&:with_examples=False" \
  -o "&:with_tests=False" \
  -o "&:with_benchmarks=False"
```

The `test_package/` consumer exercises public headers, cache/layout helpers,
headless rendering, SVG export, and runtime resource copying. Add coverage there
when package-facing API changes.

The recipe exposes standard `shared` and `fPIC` options. Conan's generated
`CMakeDeps` config also loads `texgui_copy_resources(target)` and sets
`TEXGUI_RESOURCE_DIR`, matching the installed CMake package behavior.

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
TEXGUI_VCPKG_PACKAGE_SPEC='latex-renderer[sdl2]' \
  VCPKG_ROOT=/path/to/vcpkg scripts/verify-vcpkg-overlay.sh
```

The overlay port under `packaging/vcpkg-overlay/ports/latex-renderer/` builds
from the current checkout through vcpkg's portfile flow. It also fetches the
pinned MicroTeX source archive in the portfile and passes it into CMake, so
vcpkg can keep `FETCHCONTENT_FULLY_DISCONNECTED=ON` during configure. The port
is intentionally local-development shaped: set `TEXGUI_VCPKG_SOURCE_DIR` or run
the script, which sets it for you.

Before a vcpkg registry submission:

- replace the local source path with a release archive or GitHub release ref
- fill in the registry-required source hash
- verify the installed layout follows vcpkg conventions
- test default, `raylib`, and `sdl2` feature builds

## CPack Archives

CPack is configured for local binary and source archives:

```sh
scripts/verify-cpack.sh
```

The smoke test builds headless install archives, extracts the binary package,
builds an installed `find_package()` consumer, extracts the source package, and
configures the source package again. Attach these archives to GitHub releases
only after this check is green.

## External Consumers

Run outside-repository consumer checks before publishing:

```sh
scripts/verify-external-consumers.sh
scripts/verify-path-with-spaces.sh
```

`verify-external-consumers.sh` validates both `FetchContent` and installed
`find_package()` from an archived source tree. `verify-path-with-spaces.sh`
keeps install, resource copying, and output paths honest for users who build in
directories with spaces.

## Release Routine

Run `scripts/verify-release.sh` before tagging. It validates CMake builds,
install/export consumers, generated gallery artifacts, package-manager entry
points when local tools are available, CPack archives, external consumers,
path-with-spaces handling, release archive smoke, Markdown links, and API
documentation when Doxygen is installed.
