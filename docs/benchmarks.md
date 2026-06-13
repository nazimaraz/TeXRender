# Benchmarks

The benchmark executable is intentionally simple: it measures parse/layout and
headless drawing for a few representative formulas.

## Build And Run

```sh
cmake -S . -B build -DTEXGUI_BACKEND=headless -DCMAKE_BUILD_TYPE=Release
cmake --build build --target texgui_render_benchmark -j
./build/texgui_render_benchmark
```

The output reports:

- cold render for the first parse/layout of each formula
- repeated render time over a warm process
- repeated draw time into `HeadlessSurface`

Use this for relative comparisons while optimizing cache behavior or backend
work. It is not a substitute for profiling a real application frame loop.

Current reference numbers are tracked in
[`docs/performance-baseline.md`](performance-baseline.md). The non-blocking
[`Benchmarks`](../.github/workflows/benchmarks.yml) workflow uploads raw output
for CI runs so regressions are visible during review.
