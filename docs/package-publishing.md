# Package Publishing

The repository includes starter metadata for Conan and vcpkg. Keep these paths
green before submitting to public registries.

Run the metadata gate whenever package fields change:

```sh
scripts/check-registry-readiness.py
```

The `Registry readiness` workflow runs the same check on package metadata PRs.

## pkg-config

Installed builds write `texrender.pc` next to the CMake package metadata. This is
intended for small non-CMake tools and build systems that already use
pkg-config. The complete example lives in
[`examples/consumer-pkg-config`](../examples/consumer-pkg-config):

```sh
scripts/verify-pkg-config.sh
```

The smoke test installs the headless backend into a temporary prefix, asks
pkg-config for `--cflags --libs --static texrender`, compiles a standalone C++17
consumer, and runs it. Keep this green when install paths, dependencies, or
resource-copy behavior change.

## Installed Headers And Backend Packages

The installed-header smoke test compiles exactly the headers present in an
installed prefix as C++17 and checks installed CMake package components:

```sh
scripts/verify-installed-headers.sh
```

Headless and SVG installs should not expose raylib or SDL2 headers. GUI backend
installs expose only their selected backend header after that dependency is
available through `find_package`.

The same smoke verifies:

- `find_package(texrender CONFIG REQUIRED COMPONENTS headless svg)` succeeds
- a headless/SVG install rejects `COMPONENTS raylib` and `COMPONENTS sdl2`
- a GUI backend install accepts its matching component and rejects the other GUI
  component
- `texrender_BACKEND`, `texrender_HAS_*`, and `texrender/config.hpp` agree

To validate installed GUI backend packages when local dependencies are
discoverable:

```sh
scripts/verify-backend-packages.sh
```

The script skips a backend when `find_package(raylib)` or `find_package(SDL2)`
is not available, and verifies the package when it is. Set
`TEXRENDER_BACKEND_PACKAGE_REQUIRE=1` when preparing a machine specifically to
prove GUI package exports; the script then fails instead of skipping a missing
raylib/SDL2 package.

## Shared Libraries

Dynamic-linking consumers are covered by a dedicated install smoke:

```sh
scripts/verify-shared-library.sh
```

The script builds `BUILD_SHARED_LIBS=ON`, installs the package, verifies that a
shared library was installed, compiles installed headers, checks CMake package
components, and runs both the generic package consumer and the
`examples/consumer-find-package` recipe with the runtime library path pointed at
the installed prefix. By default it uses the dependency-free headless backend;
set `TEXRENDER_SHARED_BACKEND=svg` to exercise the SVG default surface as well.

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
`CMakeDeps` config also loads `texrender_copy_resources(target)` and sets
`TEXRENDER_RESOURCE_DIR`, matching the installed CMake package behavior.

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
  -DTEXRENDER_BUILD_EXAMPLES=OFF \
  -DTEXRENDER_BUILD_BENCHMARKS=OFF
cmake --build build-vcpkg -j
ctest --test-dir build-vcpkg --output-on-failure
```

Overlay port smoke test:

```sh
VCPKG_ROOT=/path/to/vcpkg scripts/verify-vcpkg-overlay.sh
TEXRENDER_VCPKG_PACKAGE_SPEC='texrender[sdl2]' \
  VCPKG_ROOT=/path/to/vcpkg scripts/verify-vcpkg-overlay.sh
```

The overlay port under `packaging/vcpkg-overlay/ports/texrender/` builds
from the current checkout through vcpkg's portfile flow. It also fetches the
pinned MicroTeX source archive in the portfile and passes it into CMake, so
vcpkg can keep `FETCHCONTENT_FULLY_DISCONNECTED=ON` during configure. The port
is intentionally local-development shaped: set `TEXRENDER_VCPKG_SOURCE_DIR` or run
the script, which sets it for you. After install, the script also configures the
package component smoke through the vcpkg toolchain so `texrender/config.hpp`,
`find_package(texrender COMPONENTS ...)`, and selected backend features agree with
the installed port. It also checks that unavailable GUI components are rejected,
matching the installed CMake package smoke.

Before a vcpkg registry submission:

- replace the local source path with a release archive or GitHub release ref
- fill in the registry-required source hash
- verify the installed layout follows vcpkg conventions
- test default/headless, `raylib`, and `sdl2` feature builds

## CPack Archives

CPack is configured for local binary and source archives:

```sh
scripts/verify-cpack.sh
```

The smoke test builds headless install archives, extracts the binary package,
builds an installed `find_package()` consumer, extracts the source package, and
configures the source package again. Attach these archives to GitHub releases
only after this check is green. The `Release` workflow publishes CPack packages
beside the hand-named install archives, and `Release smoke` verifies CPack on
pull requests and tags.

## External Consumers

Run outside-repository consumer checks before publishing:

```sh
scripts/verify-external-consumers.sh
scripts/verify-path-with-spaces.sh
```

`verify-external-consumers.sh` validates both `FetchContent` and installed
`find_package()` from an archived source tree. `verify-path-with-spaces.sh`
keeps install, resource copying, and output paths honest for users who build in
directories with spaces. Both run in the `Release smoke` workflow.

## Release Routine

Run `scripts/verify-release.sh` before tagging. It validates CMake builds,
install/export consumers, installed public headers, generated gallery artifacts,
package-manager entry points when local tools are available, installed package
component metadata, optional installed backend package exports, pkg-config
consumers when pkg-config is available, shared-library install consumers,
CPack archives, external consumers, path-with-spaces handling, release archive
smoke, Markdown links, public API snapshot drift, and API documentation when
Doxygen is installed. It also runs the registry-readiness metadata check when
Python is available.
