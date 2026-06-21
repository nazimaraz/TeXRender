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

The regular CI workflow also runs the test suite under AddressSanitizer and
UndefinedBehaviorSanitizer.

The [`Thread sanitizer`](../.github/workflows/thread-sanitizer.yml) workflow
runs on schedule, manual dispatch, and pull requests that touch shared rendering
or threading-sensitive files. It builds focused headless tests with
`-fsanitize=thread` and runs the lifetime stress and headless render tests under
TSan.

## Local Run

```sh
scripts/verify-static-analysis.sh
```

The wrapper configures a headless Debug compile database, selects first-party
translation units from `source/`, `tests/`, `examples/`, and `benchmarks/`, and
runs clang-tidy with warnings promoted to errors. Set `CXX`, `CLANG_TIDY`,
`BUILD_DIR`, or `GENERATOR` to override the defaults.

## Local AddressSanitizer Smoke

Use the wrapper script to verify the compiler's ASan runtime before building the
project:

```sh
scripts/verify-address-sanitizer.sh
```

On macOS, the script probes the default compiler and common Homebrew LLVM paths.
Set `CXX` if you want to force a specific Clang.

## Local ThreadSanitizer Smoke

Use the wrapper script to verify the compiler's ThreadSanitizer runtime before
building the focused tests:

```sh
scripts/verify-thread-sanitizer.sh
```

On macOS, the script probes the default compiler and common Homebrew LLVM paths.
Set `CXX` if you want to force a specific Clang.

The lifetime stress executable keeps CI defaults short but can be made more
aggressive locally:

```sh
TEXRENDER_LIFETIME_THREADS=8 \
TEXRENDER_LIFETIME_THREAD_ITERATIONS=200 \
TEXRENDER_LIFETIME_OUTLIVE_ITERATIONS=500 \
TEXRENDER_LIFETIME_HANDLES_PER_DOCUMENT=24 \
  scripts/verify-thread-sanitizer.sh
```

Or use the CMake preset directly:

```sh
cmake --preset tsan
cmake --build --preset tsan --target texrender_lifetime_stress_test texrender_headless_render_test
TSAN_OPTIONS=halt_on_error=1 ./out/build/tsan/texrender_lifetime_stress_test
TSAN_OPTIONS=halt_on_error=1 ./out/build/tsan/texrender_headless_render_test
```
