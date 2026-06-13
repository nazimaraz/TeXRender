# Release Checklist

Use this before cutting a public tag.

## Local Verification

- Configure/build/test default raylib:
  `cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure`
- Configure/build/test headless:
  `cmake -S . -B build-headless -DTEXGUI_BACKEND=headless && cmake --build build-headless -j && ctest --test-dir build-headless --output-on-failure`
- Configure/build/test SVG:
  `cmake -S . -B build-svg -DTEXGUI_BACKEND=svg && cmake --build build-svg -j && ctest --test-dir build-svg --output-on-failure`
- Configure/build/test shared headless:
  `cmake -S . -B build-shared -DBUILD_SHARED_LIBS=ON -DTEXGUI_BACKEND=headless && cmake --build build-shared -j && ctest --test-dir build-shared --output-on-failure`
- Verify presets:
  `cmake --list-presets`
- Smoke-run offscreen:
  `./build-headless/texgui_offscreen 'e^{i\pi} + 1 = 0' /tmp/formula.png`
- Smoke-run SVG export:
  `./build-svg/texgui_svg_export 'e^{i\pi} + 1 = 0' /tmp/formula.svg`
- Verify install/export:
  `cmake --install build-headless --prefix /tmp/texgui-prefix`
- Verify installed package consumption:
  `cmake -S tests/package_consumer -B /tmp/texgui-package-consumer -DCMAKE_PREFIX_PATH=/tmp/texgui-prefix && cmake --build /tmp/texgui-package-consumer -j && ctest --test-dir /tmp/texgui-package-consumer --output-on-failure`
- Run the benchmark once in Release:
  `./build-headless/texgui_render_benchmark`
- Run a fuzz smoke if Clang/libFuzzer is available:
  `./build-fuzz/texgui_try_render_fuzzer -runs=1000 -max_len=256`
- Verify package-manager entry points:
  `conan create . --build=missing -o "&:backend=headless" -o "&:with_tests=False"`,
  `conan create . --build=missing -o "&:backend=svg" -o "&:with_tests=False"`,
  and a vcpkg manifest configure with `-DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON`.
- Regenerate API docs locally if Doxygen is available:
  `doxygen Doxyfile`
- Run static analysis and coverage locally when the tools are available; see
  [`docs/static-analysis.md`](static-analysis.md) and
  [`docs/coverage.md`](coverage.md).
- Run the local release verifier:
  `scripts/verify-release.sh`
- Run warnings as errors at least once:
  `cmake -S . -B build-werror -DTEXGUI_BACKEND=headless -DTEXGUI_WARNINGS_AS_ERRORS=ON && cmake --build build-werror -j`
- Lint workflows locally when actionlint is available:
  `scripts/lint-workflows.sh`

## GitHub Setup

- Enable GitHub Pages with source set to "GitHub Actions".
- Enable private vulnerability reporting in the Security tab.
- Confirm the `Package managers`, `Static analysis`, `Coverage`, `Workflow lint`,
  `Benchmarks`, `Web demo` and `Release` workflows are enabled.
- Add repository topics: `cpp`, `latex`, `renderer`, `raylib`, `svg`,
  `freetype`, `webassembly`, `cmake`, `graphics`.
- Add a short repository description:
  "C++ LaTeX math renderer for raylib, headless RGBA output, SVG, and WebAssembly."

## Release Notes

- Confirm `project(VERSION ...)`, `include/texgui/version.hpp`, `CHANGELOG.md`
  and the Git tag agree.
- Include the demo image/GitHub Pages link in the release body.
- Mention dependency modes: `FetchContent`, `add_subdirectory`, and installed
  `find_package(texgui CONFIG REQUIRED)`.
- Mention shared-library support and CMake presets.
- Mention `try_render_detailed()` for non-throwing user-input diagnostics.
- Mention `HeadlessSurface::write_png()` for offscreen image output.
- Mention `SvgSurface`, `TEXGUI_BACKEND=svg`, cache limits, and baseline layout
  helpers.
- Mention runtime resource shipping via `texgui_copy_resources(target)`.
- Mention package-manager starter files: `vcpkg.json` and `conanfile.py`.
- Let the release workflow attach install archives and `SHA256SUMS` after pushing
  a `vX.Y.Z` tag.

## Presentation

- Keep `docs/demo.png` current.
- Keep `docs/offscreen-example.png`, `docs/gallery.md` and
  `docs/performance-baseline.md` current.
- Keep [`docs/case-study.md`](case-study.md) aligned with the public API and CI
  story before using the project in a resume.
- Keep [`docs/roadmap.md`](roadmap.md) honest about near-term plans and
  non-goals.
- Optional: record a short GIF of the live editor for the README or release.
- Link `docs/architecture.md` from the release body for developer reviewers.
