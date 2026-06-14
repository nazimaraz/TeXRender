# Performance Baseline

`texrender_render_benchmark` reports cold render, repeated render, and headless
draw timings for a few representative formulas. Treat the numbers as a local
baseline, not a universal promise: fonts, CPU, compiler, sanitizer settings, and
build type all matter.

Reference run:

- Date: 2026-06-14
- Platform: macOS, Apple Silicon
- Compiler: Apple Clang
- Build: Release, `TEXRENDER_BACKEND=headless`

| Case | Metric | Time |
| --- | --- | ---: |
| short | cold render | 1.072 ms |
| short | render | 26.717 ms total, 0.134 ms/op |
| short | draw headless | 105.711 ms total, 0.529 ms/op |
| fraction | cold render | 0.490 ms |
| fraction | render | 26.824 ms total, 0.134 ms/op |
| fraction | draw headless | 344.002 ms total, 1.720 ms/op |
| long | cold render | 0.797 ms |
| long | render | 129.250 ms total, 0.646 ms/op |
| long | draw headless | 348.447 ms total, 1.742 ms/op |

Refresh the table with:

```sh
cmake -S . -B build-bench -DTEXRENDER_BACKEND=headless -DCMAKE_BUILD_TYPE=Release
cmake --build build-bench --target texrender_render_benchmark -j
./build-bench/texrender_render_benchmark
```

The machine-readable baseline used by
[`scripts/check-benchmark-baseline.py`](../scripts/check-benchmark-baseline.py)
lives in [`docs/performance-baseline.json`](performance-baseline.json). The
script compares per-operation times and defaults to a loose 1.4x threshold so it
catches obvious slowdowns without pretending local benchmark numbers are
universal.

The [`Benchmarks`](../.github/workflows/benchmarks.yml) workflow uploads the raw
output for every run and is marked non-blocking. It runs the baseline comparison
in warn-only mode, making performance movement visible without failing PRs over
normal CI noise.
