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

The [`Thread sanitizer`](../.github/workflows/thread-sanitizer.yml) workflow
runs on schedule, manual dispatch, and pull requests that touch shared rendering
or threading-sensitive files. It builds focused headless tests with
`-fsanitize=thread` and runs the lifetime stress and headless render tests under
TSan.

## Local Run

```sh
cmake -S . -B build-tidy -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DTEXRENDER_BACKEND=headless \
  -DTEXRENDER_BUILD_EXAMPLES=OFF \
  -DTEXRENDER_BUILD_TESTS=ON \
  -DTEXRENDER_BUILD_BENCHMARKS=OFF

clang-tidy -p build-tidy source/core/latex_document.cpp
```

For a full local run, generate a file list from `build-tidy/compile_commands.json`
the same way the workflow does.

## Local ThreadSanitizer Smoke

Use the wrapper script when your Clang runtime supports ThreadSanitizer:

```sh
CXX=clang++ scripts/verify-thread-sanitizer.sh
```

Or use the CMake preset directly:

```sh
cmake --preset tsan
cmake --build --preset tsan --target texrender_lifetime_stress_test texrender_headless_render_test
TSAN_OPTIONS=halt_on_error=1 ./out/build/tsan/texrender_lifetime_stress_test
TSAN_OPTIONS=halt_on_error=1 ./out/build/tsan/texrender_headless_render_test
```
