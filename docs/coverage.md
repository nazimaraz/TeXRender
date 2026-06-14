# Coverage

Coverage is measured on the headless backend because it needs no GPU, display, or
window manager. That makes it suitable for CI and for local runs on servers.

## GitHub Workflow

The [`Coverage`](../.github/workflows/coverage.yml) workflow builds with GCC
coverage instrumentation, runs the full headless test suite, and uploads:

- `coverage.xml` for tooling
- `coverage.html` and detail pages for browsing

Reports filter to `source/` and `include/`, excluding fetched dependencies and
tests.

## Local Run

```sh
cmake -S . -B build-coverage -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=g++-14 \
  -DTEXRENDER_BACKEND=headless \
  -DTEXRENDER_BUILD_EXAMPLES=OFF \
  -DTEXRENDER_BUILD_TESTS=ON \
  -DTEXRENDER_BUILD_BENCHMARKS=OFF \
  -DCMAKE_CXX_FLAGS="--coverage -O0 -g" \
  -DCMAKE_EXE_LINKER_FLAGS="--coverage"

cmake --build build-coverage -j
ctest --test-dir build-coverage --output-on-failure

gcovr --root . \
  --filter 'source/' \
  --filter 'include/' \
  --exclude '.*/_deps/.*' \
  --exclude 'tests/.*' \
  --html-details coverage.html \
  --xml-pretty --xml coverage.xml \
  --print-summary
```
