# Release Checklist

Use this before cutting a public tag.

## Local Verification

- Configure/build/test default headless:
  `cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure`
- Configure/build/test raylib when the dependency is available:
  `cmake -S . -B build-raylib -DTEXRENDER_BACKEND=raylib && cmake --build build-raylib -j && ctest --test-dir build-raylib --output-on-failure`
- Configure/build/test SVG:
  `cmake -S . -B build-svg -DTEXRENDER_BACKEND=svg && cmake --build build-svg -j && ctest --test-dir build-svg --output-on-failure`
- Configure/build/test SDL2:
  `cmake -S . -B build-sdl2 -DTEXRENDER_BACKEND=sdl2 && cmake --build build-sdl2 -j && ctest --test-dir build-sdl2 --output-on-failure`
- Configure/build/test shared headless:
  `cmake -S . -B build-shared -DBUILD_SHARED_LIBS=ON -DTEXRENDER_BACKEND=headless && cmake --build build-shared -j && ctest --test-dir build-shared --output-on-failure`
- Verify installed shared-library consumption:
  `scripts/verify-shared-library.sh`
- Verify presets:
  `cmake --list-presets`
- Smoke-run offscreen:
  `./build-headless/texrender_offscreen 'e^{i\pi} + 1 = 0' /tmp/formula.png`
- Smoke-run custom surface:
  `./build-headless/texrender_custom_surface`
- Smoke-run SVG export:
  `./build-svg/texrender_svg_export 'e^{i\pi} + 1 = 0' /tmp/formula.svg`
- Generate gallery artifacts:
  `TEXRENDER_BUILD_DIR=build-headless scripts/generate-gallery.sh docs/gallery-formulas.txt /tmp/texrender-gallery`
- Refresh committed gallery and README preview:
  `TEXRENDER_BUILD_DIR=build-headless scripts/generate-gallery.sh docs/gallery-formulas.txt docs/gallery && TEXRENDER_BUILD_DIR=build-raylib scripts/record-live-editor-demo.sh`
- Optional exact visual snapshots:
  `TEXRENDER_VISUAL_SNAPSHOT_DIR=/tmp/texrender-snapshots TEXRENDER_VISUAL_UPDATE_SNAPSHOTS=1 ctest --test-dir build-headless --output-on-failure`
  then rerun without `TEXRENDER_VISUAL_UPDATE_SNAPSHOTS`.
- Verify install/export:
  `cmake --install build-headless --prefix /tmp/texrender-prefix`
- Verify installed package consumption:
  `cmake -S tests/package_consumer -B /tmp/texrender-package-consumer -DCMAKE_PREFIX_PATH=/tmp/texrender-prefix && cmake --build /tmp/texrender-package-consumer -j && ctest --test-dir /tmp/texrender-package-consumer --output-on-failure`
- Verify installed headers:
  `scripts/verify-installed-headers.sh`
- Verify installed GUI backend packages when dependencies are available:
  `scripts/verify-backend-packages.sh`
- Fail on missing GUI package proof when using a prepared release machine:
  `RAYLIB_INSTALL_PREFIX=/tmp/raylib-prefix scripts/provision-raylib-package.sh`
  followed by
  `CMAKE_PREFIX_PATH=/tmp/raylib-prefix TEXRENDER_BACKEND_PACKAGE_REQUIRE=1 scripts/verify-backend-packages.sh`
- Confirm package components and feature metadata are covered by the installed
  header smoke: `headless`, `svg`, selected GUI backend, and rejected GUI
  backend components.
- Verify pkg-config consumption:
  `scripts/verify-pkg-config.sh`
- Run the benchmark once in Release:
  `./build-headless/texrender_render_benchmark --json /tmp/texrender-benchmark.json | tee /tmp/texrender-benchmark.txt`
- Compare benchmark output against the loose local baseline:
  `python3 scripts/check-benchmark-baseline.py --input /tmp/texrender-benchmark.json --warn-only`
- Run a fuzz smoke if Clang/libFuzzer is available:
  `./build-fuzz/texrender_try_render_fuzzer -runs=1000 -max_len=256`
- Run the focused ThreadSanitizer smoke:
  `scripts/verify-thread-sanitizer.sh`
- Verify package-manager entry points:
  `conan create . --build=missing -o "&:backend=headless" -o "&:with_tests=False"`,
  `conan create . --build=missing -o "&:backend=svg" -o "&:with_tests=False"`,
  a default vcpkg manifest configure,
  `VCPKG_ROOT=/path/to/vcpkg scripts/verify-vcpkg-overlay.sh`, and
  `TEXRENDER_VCPKG_PACKAGE_SPEC='texrender[sdl2]' VCPKG_ROOT=/path/to/vcpkg scripts/verify-vcpkg-overlay.sh`.
- Regenerate API docs locally if Doxygen is available:
  `doxygen Doxyfile`
- Run static analysis and coverage locally when the tools are available; see
  [`docs/static-analysis.md`](static-analysis.md) and
  [`docs/coverage.md`](coverage.md).
- Run the local release verifier:
  `scripts/verify-release.sh`
- Run the fresh-clone verifier:
  `scripts/verify-fresh-clone.sh`
- Run the full fresh-clone release rehearsal before the first public push:
  `TEXRENDER_FRESH_CLONE_FULL=1 scripts/verify-fresh-clone.sh`
- Run the source archive verifier:
  `scripts/verify-release-archive.sh`
- Run release asset consumer validation for any hand-built install archive:
  `scripts/verify-release-asset.sh <asset>`
- Run CPack archive validation:
  `scripts/verify-cpack.sh`
- Run external consumer validation:
  `scripts/verify-external-consumers.sh`
- Run path-with-spaces validation:
  `scripts/verify-path-with-spaces.sh`
- Check local Markdown links:
  `scripts/check-doc-links.py`
- Check package-registry metadata:
  `scripts/check-registry-readiness.py`
- Check third-party notice and redistribution docs:
  `scripts/check-third-party-notices.py`
- Check public API snapshot drift:
  `scripts/check-public-api.py`
- Run the public release preparation gate once a remote is configured:
  `scripts/prepare-public-release.sh`
- Follow the first public push dry run:
  [`docs/first-public-push-dry-run.md`](first-public-push-dry-run.md)
- Run warnings as errors at least once:
  `cmake -S . -B build-werror -DTEXRENDER_BACKEND=headless -DTEXRENDER_WARNINGS_AS_ERRORS=ON && cmake --build build-werror -j`
- Lint workflows locally when actionlint is available:
  `scripts/lint-workflows.sh`
- Review generated local artifacts before final push:
  `scripts/clean-local-artifacts.sh`

## GitHub Setup

- Enable GitHub Pages with source set to "GitHub Actions".
- Enable private vulnerability reporting in the Security tab.
- Confirm the `Package managers`, `Registry readiness`, `Static analysis`,
  `Thread sanitizer`, `Coverage`, `Workflow lint`, `CodeQL`, `Benchmarks`, `Web
  demo`, `Release smoke`, `Nightly fuzz` and `Release` workflows are enabled.
- Confirm `.github/CODEOWNERS`, `CODE_OF_CONDUCT.md`, `SUPPORT.md`,
  `CITATION.cff` and `.gitattributes` are present.
- Add repository topics: `cpp`, `latex`, `renderer`, `raylib`, `svg`,
  `sdl2`, `freetype`, `webassembly`, `cmake`, `graphics`.
- Add a short repository description:
  "C++ LaTeX math renderer for raylib, SDL2, headless RGBA output, SVG, and WebAssembly."

## Release Notes

- Confirm `project(VERSION ...)`, `include/texrender/version.hpp`, `CHANGELOG.md`
  and the Git tag agree.
- Include the demo image/GitHub Pages link in the release body.
- Mention dependency modes: `FetchContent`, `add_subdirectory`, and installed
  `find_package(texrender CONFIG REQUIRED)`.
- Mention shared-library support and CMake presets.
- Mention installed shared-library consumer verification.
- Mention `try_render_detailed()` for non-throwing user-input diagnostics.
- Mention `HeadlessSurface::write_png()` for offscreen image output.
- Mention `SvgSurface`, `TEXRENDER_BACKEND=svg`, cache limits, and baseline layout
  helpers.
- Mention `Sdl2Surface` and `TEXRENDER_BACKEND=sdl2`.
- Mention `texrender/texrender.hpp` as the backend-neutral umbrella header.
- Mention `texrender/config.hpp`, CMake package components, and `texrender_HAS_*`
  package metadata.
- Mention runtime resource shipping via `texrender_copy_resources(target)`.
- Mention package-manager starter files: `vcpkg.json` and `conanfile.py`.
- Mention third-party redistribution notes from [`THIRD_PARTY.md`](../THIRD_PARTY.md)
  and [`docs/dependency-license-audit.md`](dependency-license-audit.md).
- Mention the local MicroTeX safety patch and upstream follow-up from
  [`docs/upstream-microtex-patch.md`](upstream-microtex-patch.md) if the pinned
  MicroTeX revision still needs it.
- Mention copyable integration recipes in [`docs/recipes.md`](recipes.md).
- Mention API compatibility status and the current source-compatibility promise.
- Mention migration expectations from [`docs/migration.md`](migration.md).
- Mention CPack binary/source archives if you attach them to a release.
- Link [`docs/release-assets.md`](release-assets.md) from the release body when
  attaching multiple archive types.
- Include [`docs/support-matrix.md`](support-matrix.md) and
  [`docs/release-notes-v0.1.0.md`](release-notes-v0.1.0.md) in the release body.
- Use [`docs/release-body-v0.1.0.md`](release-body-v0.1.0.md) as the first
  GitHub release draft.
- Let the release workflow attach install archives, CPack archives and
  `SHA256SUMS` after pushing a `vX.Y.Z` tag.
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
