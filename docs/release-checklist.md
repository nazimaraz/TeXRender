# Release Checklist

Use this before cutting a public tag.

## Local Verification

- Configure/build/test default raylib:
  `cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure`
- Configure/build/test headless:
  `cmake -S . -B build-headless -DTEXGUI_BACKEND=headless && cmake --build build-headless -j && ctest --test-dir build-headless --output-on-failure`
- Configure/build/test SVG:
  `cmake -S . -B build-svg -DTEXGUI_BACKEND=svg && cmake --build build-svg -j && ctest --test-dir build-svg --output-on-failure`
- Configure/build/test SDL2:
  `cmake -S . -B build-sdl2 -DTEXGUI_BACKEND=sdl2 && cmake --build build-sdl2 -j && ctest --test-dir build-sdl2 --output-on-failure`
- Configure/build/test shared headless:
  `cmake -S . -B build-shared -DBUILD_SHARED_LIBS=ON -DTEXGUI_BACKEND=headless && cmake --build build-shared -j && ctest --test-dir build-shared --output-on-failure`
- Verify presets:
  `cmake --list-presets`
- Smoke-run offscreen:
  `./build-headless/texgui_offscreen 'e^{i\pi} + 1 = 0' /tmp/formula.png`
- Smoke-run custom surface:
  `./build-headless/texgui_custom_surface`
- Smoke-run SVG export:
  `./build-svg/texgui_svg_export 'e^{i\pi} + 1 = 0' /tmp/formula.svg`
- Generate gallery artifacts:
  `TEXGUI_BUILD_DIR=build-headless scripts/generate-gallery.sh docs/gallery-formulas.txt /tmp/texgui-gallery`
- Refresh committed gallery and README preview:
  `TEXGUI_BUILD_DIR=build-headless scripts/generate-gallery.sh docs/gallery-formulas.txt docs/gallery && TEXGUI_BUILD_DIR=build scripts/record-live-editor-demo.sh`
- Optional exact visual snapshots:
  `TEXGUI_VISUAL_SNAPSHOT_DIR=/tmp/texgui-snapshots TEXGUI_VISUAL_UPDATE_SNAPSHOTS=1 ctest --test-dir build-headless --output-on-failure`
  then rerun without `TEXGUI_VISUAL_UPDATE_SNAPSHOTS`.
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
  a vcpkg manifest configure with `-DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON`,
  and `VCPKG_ROOT=/path/to/vcpkg scripts/verify-vcpkg-overlay.sh`.
- Regenerate API docs locally if Doxygen is available:
  `doxygen Doxyfile`
- Run static analysis and coverage locally when the tools are available; see
  [`docs/static-analysis.md`](static-analysis.md) and
  [`docs/coverage.md`](coverage.md).
- Run the local release verifier:
  `scripts/verify-release.sh`
- Run the source archive verifier:
  `scripts/verify-release-archive.sh`
- Run CPack archive validation:
  `scripts/verify-cpack.sh`
- Run external consumer validation:
  `scripts/verify-external-consumers.sh`
- Run path-with-spaces validation:
  `scripts/verify-path-with-spaces.sh`
- Check local Markdown links:
  `scripts/check-doc-links.py`
- Run the public release preparation gate once a remote is configured:
  `scripts/prepare-public-release.sh`
- Run warnings as errors at least once:
  `cmake -S . -B build-werror -DTEXGUI_BACKEND=headless -DTEXGUI_WARNINGS_AS_ERRORS=ON && cmake --build build-werror -j`
- Lint workflows locally when actionlint is available:
  `scripts/lint-workflows.sh`

## GitHub Setup

- Enable GitHub Pages with source set to "GitHub Actions".
- Enable private vulnerability reporting in the Security tab.
- Confirm the `Package managers`, `Static analysis`, `Coverage`, `Workflow lint`,
  `CodeQL`, `Benchmarks`, `Web demo`, `Release smoke` and `Release` workflows
  are enabled.
- Add repository topics: `cpp`, `latex`, `renderer`, `raylib`, `svg`,
  `sdl2`, `freetype`, `webassembly`, `cmake`, `graphics`.
- Add a short repository description:
  "C++ LaTeX math renderer for raylib, SDL2, headless RGBA output, SVG, and WebAssembly."

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
- Mention `Sdl2Surface` and `TEXGUI_BACKEND=sdl2`.
- Mention runtime resource shipping via `texgui_copy_resources(target)`.
- Mention package-manager starter files: `vcpkg.json` and `conanfile.py`.
- Mention API compatibility status and the current source-compatibility promise.
- Mention migration expectations from [`docs/migration.md`](migration.md).
- Mention CPack binary/source archives if you attach them to a release.
- Include [`docs/support-matrix.md`](support-matrix.md) and
  [`docs/release-notes-v0.1.0.md`](release-notes-v0.1.0.md) in the release body.
- Let the release workflow attach install archives and `SHA256SUMS` after pushing
  a `vX.Y.Z` tag.
- Follow [`docs/github-launch.md`](github-launch.md) for first push and
  branch-protection setup.
- Follow [`docs/registry-submissions.md`](registry-submissions.md) for vcpkg and
  Conan Center submission work after the first public tag.

## Presentation

- Keep `docs/demo.png` current.
- Keep `docs/offscreen-example.png`, `docs/gallery.md`, `docs/live-preview.gif`
  and `docs/performance-baseline.md` current. Generated gallery artifacts should
  come from `scripts/generate-gallery.sh`; the README GIF should come from the
  real live-editor capture script, `scripts/record-live-editor-demo.sh`. Use
  `scripts/generate-preview-gif.sh` only when a display-free fallback is needed.
- Keep [`docs/case-study.md`](case-study.md) aligned with the public API and CI
  story before using the project in a resume.
- Keep [`docs/roadmap.md`](roadmap.md) honest about near-term plans and
  non-goals.
- Include the live editor GIF in the README or release body when it has been
  refreshed from the release commit.
- Link `docs/architecture.md` from the release body for developer reviewers.
