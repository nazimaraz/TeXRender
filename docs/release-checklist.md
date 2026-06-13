# Release Checklist

Use this before cutting a public tag.

## Local Verification

- Configure/build/test default raylib:
  `cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure`
- Configure/build/test headless:
  `cmake -S . -B build-headless -DTEXGUI_BACKEND=headless && cmake --build build-headless -j && ctest --test-dir build-headless --output-on-failure`
- Smoke-run offscreen:
  `./build-headless/texgui_offscreen 'e^{i\pi} + 1 = 0' /tmp/formula.ppm`
- Verify install/export:
  `cmake --install build-headless --prefix /tmp/texgui-prefix`
- Verify installed package consumption:
  `cmake -S tests/package_consumer -B /tmp/texgui-package-consumer -DCMAKE_PREFIX_PATH=/tmp/texgui-prefix && cmake --build /tmp/texgui-package-consumer -j && ctest --test-dir /tmp/texgui-package-consumer --output-on-failure`
- Run the benchmark once in Release:
  `./build-headless/texgui_render_benchmark`
- Run a fuzz smoke if Clang/libFuzzer is available:
  `./build-fuzz/texgui_try_render_fuzzer -runs=1000 -max_len=256`
- Verify package-manager entry points:
  `conan create . --build=missing -o "&:backend=headless" -o "&:with_tests=False"`
  and a vcpkg manifest configure with `-DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON`.
- Regenerate API docs locally if Doxygen is available:
  `doxygen Doxyfile`

## GitHub Setup

- Enable GitHub Pages with source set to "GitHub Actions".
- Enable private vulnerability reporting in the Security tab.
- Confirm the `Package managers`, `Benchmarks`, `Web demo` and `Release`
  workflows are enabled.
- Add repository topics: `cpp`, `latex`, `renderer`, `raylib`, `freetype`,
  `webassembly`, `cmake`, `graphics`.
- Add a short repository description:
  "C++ LaTeX math renderer for raylib, headless RGBA output, and WebAssembly."

## Release Notes

- Confirm `project(VERSION ...)`, `include/texgui/version.hpp`, `CHANGELOG.md`
  and the Git tag agree.
- Include the demo image/GitHub Pages link in the release body.
- Mention dependency modes: `FetchContent`, `add_subdirectory`, and installed
  `find_package(texgui CONFIG REQUIRED)`.
- Mention runtime resource shipping via `texgui_copy_resources(target)`.
- Mention package-manager starter files: `vcpkg.json` and `conanfile.py`.
- Let the release workflow attach install archives and `SHA256SUMS` after pushing
  a `vX.Y.Z` tag.

## Presentation

- Keep `docs/demo.png` current.
- Keep `docs/offscreen-example.png`, `docs/gallery.md` and
  `docs/performance-baseline.md` current.
- Optional: record a short GIF of the live editor for the README or release.
- Link `docs/architecture.md` from the release body for developer reviewers.
