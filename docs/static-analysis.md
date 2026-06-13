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
