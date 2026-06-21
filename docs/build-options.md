# Build Options

This project is usable as a top-level application/demo repo and as an embedded
CMake dependency. The options below are the supported switches contributors and
consumers are expected to use.

## CMake Presets

Common local configurations are available through `CMakePresets.json`:

```sh
cmake --preset headless-debug
cmake --build --preset headless-debug
ctest --preset headless-debug
```

Useful presets:

- `headless-debug`: windowless Debug build with warnings as errors.
- `headless-release`: windowless Release build.
- `raylib-release`: raylib GUI backend and examples.
- `sdl2-release`: SDL2 GUI backend and example.
- `headless-shared`: shared-library build using the headless backend.
- `svg-release`: SVG backend and export example.
- `asan`: AddressSanitizer + UndefinedBehaviorSanitizer.
- `tsan`: ThreadSanitizer headless build.
- `coverage`: coverage-instrumented headless build.
- `fuzz`: libFuzzer target for `try_render()`.

## Library Options

- `TEXRENDER_BACKEND`: `raylib`, `sdl2`, `headless`, or `svg`. The selected backend
  controls the default surface created by `LatexDocument`; `HeadlessSurface` and
  `SvgSurface` are available in every build for injection.
- `TEXRENDER_BUILD_EXAMPLES`: build example executables. Defaults on only when this
  project is top-level.
- `TEXRENDER_BUILD_TESTS`: build tests. Defaults on only when this project is
  top-level.
- `TEXRENDER_BUILD_BENCHMARKS`: build benchmark executables. Defaults on only when
  this project is top-level.
- `TEXRENDER_BUILD_FUZZERS`: build libFuzzer targets. Defaults off.
- `TEXRENDER_FETCH_GUI_DEPENDENCIES`: when `TEXRENDER_BACKEND=raylib` or `sdl2`,
  fetch the missing GUI dependency from source after `find_package()` fails.
  Defaults off so dependency consumers fail fast instead of unexpectedly
  downloading/building a GUI toolkit during configure.
- `TEXRENDER_INSTALL`: generate install/export support. Defaults off when FreeType,
  tinyxml2, raylib, or SDL2 is fetched from source because the installed package
  relies on `find_dependency()` to recreate those targets for consumers.
- `TEXRENDER_WARNINGS_AS_ERRORS`: treat first-party warnings as errors. This never
  applies to MicroTeX or other third-party sources.

On macOS top-level builds, `CMAKE_OSX_DEPLOYMENT_TARGET` defaults to the host
major.minor version when neither CMake nor `MACOSX_DEPLOYMENT_TARGET` has set
one. Set it explicitly when you need to target older macOS releases.

For local ASan/UBSan runs, prefer the wrapper script because it first verifies
that the selected compiler can run its sanitizer runtime:

```sh
scripts/verify-address-sanitizer.sh
```

On macOS with Apple CommandLineTools, the script will try common Homebrew LLVM
paths after the default `clang++`. Set `CXX` explicitly to force a particular
compiler.

Runtime cache limits are controlled from C++ with `TeXRender::set_cache_limits()`.
The CPU glyph limit is process-wide; the GPU texture limit is used by newly
created `RaylibSurface` instances and can also be set per surface.

## Public Headers

Use `#include <texrender/texrender.hpp>` for the backend-neutral public API. It includes
`texrender/config.hpp`, document/render types, cache options, layout helpers,
`HeadlessSurface`, `SvgSurface`, and version constants.

Backend-specific headers are deliberately opt-in. Installed packages include
`raylib_surface.hpp` only for `TEXRENDER_BACKEND=raylib` packages and
`sdl2_surface.hpp` only for `TEXRENDER_BACKEND=sdl2` packages. This keeps a
headless or SVG install usable without GUI development headers.

`texrender/config.hpp` exposes compile-time package features:

```cpp
#include <texrender/config.hpp>

static_assert(TEXRENDER_HAS_HEADLESS == 1);
static_assert(TEXRENDER_HAS_SVG == 1);
if constexpr (TeXRender::has_sdl2) {
    // This package was built with TEXRENDER_BACKEND=sdl2.
}
```

Installed CMake packages expose matching metadata and components:

```cmake
find_package(texrender CONFIG REQUIRED COMPONENTS headless svg)

message(STATUS "texrender backend: ${texrender_BACKEND}")
message(STATUS "SDL2 support: ${texrender_HAS_SDL2}")
```

Supported components are `headless`, `svg`, `raylib`, and `sdl2`. Requesting a
GUI component that the installed package was not built with fails during
`find_package()`.

## SDL2 Backend

`TEXRENDER_BACKEND=sdl2` builds `TeXRender::Sdl2Surface` and the
`texrender_sdl2_minimal` example. SDL2 applications can either set a process
default renderer before constructing `LatexDocument`:

```cpp
TeXRender::set_default_sdl_renderer(renderer);
TeXRender::LatexDocument document(TEXRENDER_RES_DIR);
```

or inject an explicit surface:

```cpp
auto surface = std::make_unique<TeXRender::Sdl2Surface>(renderer);
TeXRender::LatexDocument document(TEXRENDER_RES_DIR, std::move(surface));
```

The renderer is borrowed. Destroy documents and handles before destroying the
SDL renderer so cached glyph textures can be released while SDL is still alive.

## Shared Libraries

`BUILD_SHARED_LIBS=ON` builds `texrender` as a shared library. Public classes use
`TEXRENDER_EXPORT`, and consumers of a Windows DLL receive `TEXRENDER_SHARED` from the
exported target automatically.

```sh
cmake -S . -B build-shared -DBUILD_SHARED_LIBS=ON -DTEXRENDER_BACKEND=headless
cmake --build build-shared -j
ctest --test-dir build-shared --output-on-failure
```

For install/package coverage, run:

```sh
scripts/verify-shared-library.sh
```

That smoke builds and installs a shared package, compiles installed public
headers, verifies package components, and runs external `find_package()`
consumers with the runtime library path pointed at the installed prefix. The
release verifier includes both the in-tree shared build and this installed
shared-library package check.

## Optional Visual Snapshots

`visual_regression` is structural by default so it stays portable across
platforms and font rasterizer differences. For exact local PNG comparisons, set
`TEXRENDER_VISUAL_SNAPSHOT_DIR` when running the tests. To intentionally refresh
snapshots, also set `TEXRENDER_VISUAL_UPDATE_SNAPSHOTS=1`.
