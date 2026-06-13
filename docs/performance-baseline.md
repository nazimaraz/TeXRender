# Performance Baseline

`texgui_render_benchmark` reports cold render, repeated render, and headless
draw timings for a few representative formulas. Treat the numbers as a local
baseline, not a universal promise: fonts, CPU, compiler, sanitizer settings, and
build type all matter.

Reference run:

- Date: 2026-06-13
- Platform: macOS, Apple Silicon
- Compiler: Apple Clang
- Build: Release, `TEXGUI_BACKEND=headless`

| Case | Metric | Time |
| --- | --- | ---: |
| short | cold render | 0.354 ms |
| short | render | 30.940 ms total, 0.155 ms/op |
| short | draw headless | 604.316 ms total, 3.022 ms/op |
| fraction | cold render | 0.168 ms |
| fraction | render | 41.804 ms total, 0.209 ms/op |
| fraction | draw headless | 749.478 ms total, 3.747 ms/op |
| long | cold render | 4.967 ms |
| long | render | 233.650 ms total, 1.168 ms/op |
| long | draw headless | 1337.617 ms total, 6.688 ms/op |

Refresh the table with:

```sh
cmake -S . -B build-bench -DTEXGUI_BACKEND=headless -DCMAKE_BUILD_TYPE=Release
cmake --build build-bench --target texgui_render_benchmark -j
./build-bench/texgui_render_benchmark
```

The [`Benchmarks`](../.github/workflows/benchmarks.yml) workflow uploads the raw
output for every run and is marked non-blocking. It is meant to make performance
movement visible, not to fail PRs over normal CI noise.
