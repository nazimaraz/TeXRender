# Static Analysis

The repository includes a `.clang-tidy` profile focused on bug-prone patterns,
portability, performance, readability, and C++ Core Guidelines checks that fit
this codebase.

## GitHub Workflow

The [`Static analysis`](../.github/workflows/static-analysis.yml) workflow:

- installs `clang-tidy-22`
- configures a headless Debug build with `CMAKE_EXPORT_COMPILE_COMMANDS=ON`
- selects project translation units from `source/`, `tests/`, `examples/`, and
  `benchmarks/`
- runs clang-tidy with warnings promoted to errors

MicroTeX and other fetched dependencies are intentionally excluded.

The [`CodeQL`](../.github/workflows/codeql.yml) workflow adds GitHub's semantic
C/C++ analysis on pushes, pull requests, weekly schedule, and manual dispatch.
It uses a manual headless CMake build so CodeQL observes the same project sources
that normal CI compiles.

The [`Thread sanitizer`](../.github/workflows/thread-sanitizer.yml) workflow is
scheduled/manual. It builds focused headless tests with `-fsanitize=thread` and
runs the lifetime stress and headless render tests under TSan. It is kept out of
normal PR CI so contributor feedback stays fast while still exercising shared
rendering paths regularly.

## Local Run

```sh
cmake -S . -B build-tidy -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DTEXGUI_BACKEND=headless \
  -DTEXGUI_BUILD_EXAMPLES=OFF \
  -DTEXGUI_BUILD_TESTS=ON \
  -DTEXGUI_BUILD_BENCHMARKS=OFF

clang-tidy -p build-tidy source/core/latex_document.cpp
```

For a full local run, generate a file list from `build-tidy/compile_commands.json`
the same way the workflow does.

## Local ThreadSanitizer Smoke

```sh
cmake -S . -B build-tsan -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DTEXGUI_BACKEND=headless \
  -DTEXGUI_BUILD_EXAMPLES=OFF \
  -DTEXGUI_BUILD_BENCHMARKS=OFF \
  -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
cmake --build build-tsan --target texgui_lifetime_stress_test texgui_headless_render_test -j
TSAN_OPTIONS=halt_on_error=1 ./build-tsan/texgui_lifetime_stress_test
TSAN_OPTIONS=halt_on_error=1 ./build-tsan/texgui_headless_render_test
```
