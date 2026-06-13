# Fuzzing

texgui includes a libFuzzer harness for caller-supplied LaTeX input:
`tests/fuzz/try_render_fuzzer.cpp`.

## Build

Use Clang with the headless backend:

```sh
cmake -S . -B build-fuzz -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DTEXGUI_BACKEND=headless \
  -DTEXGUI_BUILD_EXAMPLES=OFF \
  -DTEXGUI_BUILD_TESTS=OFF \
  -DTEXGUI_BUILD_BENCHMARKS=OFF \
  -DTEXGUI_BUILD_FUZZERS=ON
cmake --build build-fuzz --target texgui_try_render_fuzzer -j
```

On macOS, use upstream LLVM Clang, for example Homebrew's
`/opt/homebrew/opt/llvm/bin/clang++`. AppleClang often omits the libFuzzer
runtime and is rejected at configure time.

## Run

Short smoke:

```sh
./build-fuzz/texgui_try_render_fuzzer -runs=1000 -max_len=256
```

Longer local campaign:

```sh
mkdir -p fuzz-corpus
cp tests/fuzz/corpus/* fuzz-corpus/
./build-fuzz/texgui_try_render_fuzzer fuzz-corpus -max_len=4096 -jobs=4 -workers=4
```

Useful sanitizer settings:

```sh
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1 \
  ./build-fuzz/texgui_try_render_fuzzer fuzz-corpus -max_len=4096
```

Leak detection is usually disabled because texgui and MicroTeX keep
process-lifetime singletons.

## GitHub Nightly Run

The [`Nightly fuzz`](../.github/workflows/nightly-fuzz.yml) workflow runs the
same harness on a seeded corpus every night and can be launched manually with a
custom time budget. It uploads the evolved corpus and any crash artifacts so a
failure can be reproduced locally.

## Scope

The harness feeds arbitrary bytes through `LatexDocument::try_render()` with
small `RenderOptions` limits. Successful parses are drawn into a headless canvas,
so it covers parsing, layout, glyph lookup and software compositing.
