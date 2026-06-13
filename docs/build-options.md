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
- `raylib-release`: default GUI backend and examples.
- `sdl2-release`: SDL2 GUI backend and example.
- `headless-shared`: shared-library build using the headless backend.
- `svg-release`: SVG backend and export example.
- `asan`: AddressSanitizer + UndefinedBehaviorSanitizer.
- `coverage`: coverage-instrumented headless build.
- `fuzz`: libFuzzer target for `try_render()`.

## Library Options

- `TEXGUI_BACKEND`: `raylib`, `sdl2`, `headless`, or `svg`. The selected backend
  controls the default surface created by `LatexDocument`; `HeadlessSurface` and
  `SvgSurface` are available in every build for injection.
- `TEXGUI_BUILD_EXAMPLES`: build example executables. Defaults on only when this
  project is top-level.
- `TEXGUI_BUILD_TESTS`: build tests. Defaults on only when this project is
  top-level.
- `TEXGUI_BUILD_BENCHMARKS`: build benchmark executables. Defaults on only when
  this project is top-level.
- `TEXGUI_BUILD_FUZZERS`: build libFuzzer targets. Defaults off.
- `TEXGUI_INSTALL`: generate install/export support. Defaults off when raylib or
  SDL2 is fetched from source because those targets are not cleanly exportable.
- `TEXGUI_WARNINGS_AS_ERRORS`: treat first-party warnings as errors. This never
  applies to MicroTeX or other third-party sources.

Runtime cache limits are controlled from C++ with `TexGui::set_cache_limits()`.
The CPU glyph limit is process-wide; the GPU texture limit is used by newly
created `RaylibSurface` instances and can also be set per surface.

## Public Headers

Use `#include <texgui/texgui.hpp>` for the backend-neutral public API. It includes
`texgui/config.hpp`, document/render types, cache options, layout helpers,
`HeadlessSurface`, `SvgSurface`, and version constants.

Backend-specific headers are deliberately opt-in. Installed packages include
`raylib_surface.hpp` only for `TEXGUI_BACKEND=raylib` packages and
`sdl2_surface.hpp` only for `TEXGUI_BACKEND=sdl2` packages. This keeps a
headless or SVG install usable without GUI development headers.

`texgui/config.hpp` exposes compile-time package features:

```cpp
#include <texgui/config.hpp>

static_assert(TEXGUI_HAS_HEADLESS == 1);
static_assert(TEXGUI_HAS_SVG == 1);
if constexpr (TexGui::has_sdl2) {
    // This package was built with TEXGUI_BACKEND=sdl2.
}
```

Installed CMake packages expose matching metadata and components:

```cmake
find_package(texgui CONFIG REQUIRED COMPONENTS headless svg)

message(STATUS "texgui backend: ${texgui_BACKEND}")
message(STATUS "SDL2 support: ${texgui_HAS_SDL2}")
```

Supported components are `headless`, `svg`, `raylib`, and `sdl2`. Requesting a
GUI component that the installed package was not built with fails during
`find_package()`.

## SDL2 Backend

`TEXGUI_BACKEND=sdl2` builds `TexGui::Sdl2Surface` and the
`texgui_sdl2_minimal` example. SDL2 applications can either set a process
default renderer before constructing `LatexDocument`:

```cpp
TexGui::set_default_sdl_renderer(renderer);
TexGui::LatexDocument document(TEXGUI_RES_DIR);
```

or inject an explicit surface:

```cpp
auto surface = std::make_unique<TexGui::Sdl2Surface>(renderer);
TexGui::LatexDocument document(TEXGUI_RES_DIR, std::move(surface));
```

The renderer is borrowed. Destroy documents and handles before destroying the
SDL renderer so cached glyph textures can be released while SDL is still alive.

## Shared Libraries

`BUILD_SHARED_LIBS=ON` builds `texgui` as a shared library. Public classes use
`TEXGUI_EXPORT`, and consumers of a Windows DLL receive `TEXGUI_SHARED` from the
exported target automatically.

```sh
cmake -S . -B build-shared -DBUILD_SHARED_LIBS=ON -DTEXGUI_BACKEND=headless
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
`TEXGUI_VISUAL_SNAPSHOT_DIR` when running the tests. To intentionally refresh
snapshots, also set `TEXGUI_VISUAL_UPDATE_SNAPSHOTS=1`.
