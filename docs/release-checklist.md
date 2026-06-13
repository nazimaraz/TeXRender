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

## GitHub Setup

- Enable GitHub Pages with source set to "GitHub Actions".
- Enable private vulnerability reporting in the Security tab.
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

## Presentation

- Keep `docs/demo.png` current.
- Optional: record a short GIF of the live editor for the README or release.
- Link `docs/architecture.md` from the release body for developer reviewers.
