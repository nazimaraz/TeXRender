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
- `headless-shared`: shared-library build using the headless backend.
- `asan`: AddressSanitizer + UndefinedBehaviorSanitizer.
- `coverage`: coverage-instrumented headless build.
- `fuzz`: libFuzzer target for `try_render()`.

## Library Options

- `TEXGUI_BACKEND`: `raylib` or `headless`. The selected backend controls the
  default surface created by `LatexDocument`; `HeadlessSurface` is available in
  every build.
- `TEXGUI_BUILD_EXAMPLES`: build example executables. Defaults on only when this
  project is top-level.
- `TEXGUI_BUILD_TESTS`: build tests. Defaults on only when this project is
  top-level.
- `TEXGUI_BUILD_BENCHMARKS`: build benchmark executables. Defaults on only when
  this project is top-level.
- `TEXGUI_BUILD_FUZZERS`: build libFuzzer targets. Defaults off.
- `TEXGUI_INSTALL`: generate install/export support. Defaults off when raylib is
  fetched from source because that target is not cleanly exportable.
- `TEXGUI_WARNINGS_AS_ERRORS`: treat first-party warnings as errors. This never
  applies to MicroTeX or other third-party sources.

## Shared Libraries

`BUILD_SHARED_LIBS=ON` builds `texgui` as a shared library. Public classes use
`TEXGUI_EXPORT`, and consumers of a Windows DLL receive `TEXGUI_SHARED` from the
exported target automatically.

```sh
cmake -S . -B build-shared -DBUILD_SHARED_LIBS=ON -DTEXGUI_BACKEND=headless
cmake --build build-shared -j
ctest --test-dir build-shared --output-on-failure
```

The release verifier includes this shared-library smoke test.
