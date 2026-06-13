# latex-renderer v0.1.0

`v0.1.0` is the first public release of latex-renderer, a C++ library for
embedding LaTeX-style math rendering into caller-owned render loops.

## Highlights

- Render math formulas through MicroTeX without spawning a TeX subprocess.
- Draw into raylib, SDL2, a dependency-free headless RGBA surface, or
  self-contained SVG output.
- Use a small public API built around `TexGui::LatexDocument`,
  `TexGui::RenderHandle`, `TexGui::IDrawSurface`, and `TexGui::RenderOptions`.
- Include the backend-neutral API through `texgui/texgui.hpp`; GUI backend
  headers are packaged only with installs that export their matching dependency.
- Consume the library with `FetchContent`, `add_subdirectory`, installed
  `find_package(texgui CONFIG REQUIRED)`, pkg-config metadata, Conan metadata,
  or vcpkg metadata.
- Build native demos and the same live editor as a WebAssembly/GitHub Pages app.
- Validate downstream dependency use with installed package consumers, external
  consumers, CPack smoke tests, path-with-spaces tests, and release archive
  smoke tests.

## Try It

- Live editor: https://nazimaraz.github.io/latex-renderer/
- API docs: https://nazimaraz.github.io/latex-renderer/api/
- Integration examples:
  https://github.com/nazimaraz/latex-renderer/blob/v0.1.0/docs/integration-examples.md
- Security threat model:
  https://github.com/nazimaraz/latex-renderer/blob/v0.1.0/docs/security-threat-model.md
- Dependency/license audit:
  https://github.com/nazimaraz/latex-renderer/blob/v0.1.0/docs/dependency-license-audit.md
- Release asset guide:
  https://github.com/nazimaraz/latex-renderer/blob/v0.1.0/docs/release-assets.md

## Install

Fetch directly from CMake:

```cmake
include(FetchContent)
FetchContent_Declare(
    texgui
    GIT_REPOSITORY https://github.com/nazimaraz/latex-renderer.git
    GIT_TAG        v0.1.0
)
FetchContent_MakeAvailable(texgui)

target_link_libraries(my_app PRIVATE texgui::texgui)
texgui_copy_resources(my_app)
```

Or install and consume the exported package:

```sh
cmake -S . -B build -DTEXGUI_BACKEND=headless -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cmake --install build --prefix /path/to/prefix
```

```cmake
find_package(texgui CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE texgui::texgui)
texgui_copy_resources(my_app)
```

## Release Assets

Use the source archive or tagged repository for `FetchContent` and
`add_subdirectory`. Use the platform install archives when you want a prebuilt
headless package with headers, libraries, CMake config files, and MicroTeX
resources already arranged.

Expected release assets:

- `texgui-headless-linux-x64.tar.gz`
- `texgui-headless-macos-arm64.tar.gz`
- `texgui-headless-windows-x64.zip`
- CPack binary archives
- CPack source archive
- `SHA256SUMS`

Verify downloaded archives before unpacking:

```sh
shasum -a 256 -c SHA256SUMS
```

## Compatibility

latex-renderer is pre-1.0. The `0.1.x` line is source-compatibility focused for
documented public headers and CMake targets. ABI stability is not promised before
`1.0`; rebuild consumers when upgrading.

Public headers are validated as C++17. The implementation uses C++23 internally.

See:

- https://github.com/nazimaraz/latex-renderer/blob/v0.1.0/docs/api-compatibility.md
- https://github.com/nazimaraz/latex-renderer/blob/v0.1.0/docs/migration.md
- https://github.com/nazimaraz/latex-renderer/blob/v0.1.0/docs/support-matrix.md

## Known Limits

- This is a formula renderer, not a full TeX document processor.
- MicroTeX resources are required at runtime.
- GUI backends inherit their toolkit lifetime/threading rules.
- `try_render()` guardrails help with caller-supplied input, but they are not a
  security sandbox.
- SDL2 rounded rectangles currently fall back to regular rectangles.

See
https://github.com/nazimaraz/latex-renderer/blob/v0.1.0/docs/known-limitations.md
for the full boundary.

## Validation

This release is designed to be verified before publishing with:

```sh
scripts/verify-release.sh
TEXGUI_FRESH_CLONE_FULL=1 scripts/verify-fresh-clone.sh
scripts/verify-release-archive.sh
scripts/verify-cpack.sh
scripts/verify-external-consumers.sh
scripts/verify-installed-headers.sh
scripts/verify-backend-packages.sh
scripts/verify-path-with-spaces.sh
scripts/verify-pkg-config.sh
scripts/check-registry-readiness.py
```

CI also covers Linux, macOS, Windows, ASan/UBSan, ThreadSanitizer smoke,
nightly fuzzing, CodeQL, clang-tidy, coverage, package-manager validation,
WebAssembly, workflow linting, and release smoke tests.
