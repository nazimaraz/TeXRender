# Contributing

Thanks for your interest in improving latex-renderer!

## Building and testing

```sh
# raylib backend (default) — also builds the examples
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure

# headless backend — no GPU, display, or raylib needed
cmake -S . -B build-headless -DTEXGUI_BACKEND=headless
cmake --build build-headless -j
ctest --test-dir build-headless --output-on-failure
```

The full test suite runs windowless under either backend (the end-to-end and
visual regression tests inject a `HeadlessSurface`). It also configures a tiny
external CMake consumer to prove the dependency story stays intact. Please make
sure both configurations build and pass before opening a PR, and add or extend a
test under `tests/` when you change behaviour.

CMake presets cover the common local workflows:

```sh
cmake --preset headless-debug
cmake --build --preset headless-debug
ctest --preset headless-debug
```

See [`docs/build-options.md`](docs/build-options.md) for all presets, shared
library builds and project options.

Optional developer checks:

```sh
cmake --build build-headless --target texgui_render_benchmark -j
./build-headless/texgui_render_benchmark

cmake -S . -B build-fuzz -DTEXGUI_BACKEND=headless -DTEXGUI_BUILD_FUZZERS=ON \
  -DTEXGUI_BUILD_EXAMPLES=OFF -DTEXGUI_BUILD_TESTS=OFF -DTEXGUI_BUILD_BENCHMARKS=OFF \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build-fuzz --target texgui_try_render_fuzzer -j
./build-fuzz/texgui_try_render_fuzzer -runs=1000 -max_len=256
```

Static analysis and coverage are documented in
[`docs/static-analysis.md`](docs/static-analysis.md) and
[`docs/coverage.md`](docs/coverage.md). They run in CI, but local runs are useful
before touching shared rendering or packaging code.

For a full pre-release local pass, run:

```sh
scripts/verify-release.sh
```

Workflow linting is documented in
[`docs/workflow-linting.md`](docs/workflow-linting.md).

## Code style

* Implementation files are C++23; the public headers in `include/texgui/` must
  stay **C++17-compatible** — that is the standard consumers build against.
* Formatting is enforced in CI with **clang-format 22** and the repo's
  [`.clang-format`](.clang-format). Before committing:

  ```sh
  git ls-files '*.cpp' '*.hpp' | xargs clang-format -i
  ```

* A [`.clang-tidy`](.clang-tidy) configuration is enforced in CI for project
  translation units.
* `TEXGUI_WARNINGS_AS_ERRORS=ON` enables the same first-party warning policy used
  by the strict CI job. Third-party code is still treated separately.
* GitHub Actions workflows are linted with actionlint in CI.
* Match the existing naming: `lower_case` functions and variables, `CamelCase`
  types, `trailing_underscore_` for private members.

## Adding a rendering backend

An out-of-tree backend needs no changes here at all — implement `IDrawSurface`
and inject it into `LatexDocument` (see "Adding a backend" in the README). For
an in-tree backend: put the sources plus a small `make_surface` factory file
under `source/backend/<name>/`, add a `TEXGUI_BACKEND` branch in
`CMakeLists.txt`, and extend CI if it can build headlessly.

## Pull requests

* Keep PRs focused; separate refactors from behaviour changes.
* CI must be green: the format check plus builds and tests on Linux, macOS and
  Windows.
* By contributing you agree that your work is licensed under the project's
  [MIT License](LICENSE).
