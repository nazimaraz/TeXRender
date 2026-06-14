# TeXRender v0.1.0

`v0.1.0` is the first public release of TeXRender, a C++ library for
embedding LaTeX-style math rendering into caller-owned render loops.

## Highlights

- Render math formulas through MicroTeX without spawning a TeX subprocess.
- Draw into raylib, SDL2, a dependency-free headless RGBA surface, or
  self-contained SVG output.
- Use a small public API built around `TeXRender::LatexDocument`,
  `TeXRender::RenderHandle`, `TeXRender::IDrawSurface`, and `TeXRender::RenderOptions`.
- Include the backend-neutral API through `texrender/texrender.hpp`; GUI backend
  headers are packaged only with installs that export their matching dependency.
- Inspect backend support through `texrender/config.hpp`, `texrender_BACKEND`,
  `texrender_HAS_*`, and CMake package components.
- Consume the library with `FetchContent`, `add_subdirectory`, installed
  `find_package(texrender CONFIG REQUIRED)`, pkg-config metadata, Conan metadata,
  or vcpkg metadata.
- Build static or shared packages; release smoke verifies an installed shared
  package through an external `find_package()` consumer.
- Build native demos and the same live editor as a WebAssembly/GitHub Pages app.
- Validate downstream dependency use with installed package consumers, external
  consumers, CPack smoke tests, path-with-spaces tests, and release archive
  smoke tests.
- Wrap parsed formulas inside app/UI objects with the new formula-label example,
  and track large benchmark movement with a loose baseline comparison.

## Try It

- Live editor: https://nazimaraz.github.io/TeXRender/
- API docs: https://nazimaraz.github.io/TeXRender/api/
- Integration examples:
  https://github.com/nazimaraz/TeXRender/blob/v0.1.0/docs/integration-examples.md
- Copyable recipes:
  https://github.com/nazimaraz/TeXRender/blob/v0.1.0/docs/recipes.md
- Security threat model:
  https://github.com/nazimaraz/TeXRender/blob/v0.1.0/docs/security-threat-model.md
- Dependency/license audit:
  https://github.com/nazimaraz/TeXRender/blob/v0.1.0/docs/dependency-license-audit.md
- MicroTeX patch note:
  https://github.com/nazimaraz/TeXRender/blob/v0.1.0/docs/upstream-microtex-patch.md
- Release asset guide:
  https://github.com/nazimaraz/TeXRender/blob/v0.1.0/docs/release-assets.md

## Install

Fetch directly from CMake:

```cmake
include(FetchContent)
set(TEXRENDER_BACKEND headless CACHE STRING "" FORCE)
set(TEXRENDER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TEXRENDER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    texrender
    GIT_REPOSITORY https://github.com/nazimaraz/TeXRender.git
    GIT_TAG        v0.1.0
)
FetchContent_MakeAvailable(texrender)

target_link_libraries(my_app PRIVATE texrender::texrender)
texrender_copy_resources(my_app)
```

Or install and consume the exported package:

```sh
cmake -S . -B build -DTEXRENDER_BACKEND=headless -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cmake --install build --prefix /path/to/prefix
```

```cmake
find_package(texrender CONFIG REQUIRED COMPONENTS headless svg)
target_link_libraries(my_app PRIVATE texrender::texrender)
texrender_copy_resources(my_app)
```

## Release Assets

Use the source archive or tagged repository for `FetchContent` and
`add_subdirectory`. Use the platform install archives when you want a prebuilt
headless package with headers, libraries, CMake config files, and MicroTeX
resources already arranged.

Expected release assets:

- `texrender-headless-linux-x64.tar.gz`
- `texrender-headless-macos-arm64.tar.gz`
- `texrender-headless-windows-x64.zip`
- CPack binary archives
- CPack source archive
- `SHA256SUMS`

Verify downloaded archives before unpacking:

```sh
shasum -a 256 -c SHA256SUMS
```

## Compatibility

TeXRender is pre-1.0. The `0.1.x` line is source-compatibility focused for
documented public headers and CMake targets. ABI stability is not promised before
`1.0`; rebuild consumers when upgrading.

The implementation and public headers are validated as C++17.

See:

- https://github.com/nazimaraz/TeXRender/blob/v0.1.0/docs/api-compatibility.md
- https://github.com/nazimaraz/TeXRender/blob/v0.1.0/docs/migration.md
- https://github.com/nazimaraz/TeXRender/blob/v0.1.0/docs/support-matrix.md

## Known Limits

- This is a formula renderer, not a full TeX document processor.
- MicroTeX resources are required at runtime.
- GUI backends inherit their toolkit lifetime/threading rules.
- `try_render()` guardrails help with caller-supplied input, but they are not a
  security sandbox.
- SDL2 rounded rectangles currently fall back to regular rectangles.

See
https://github.com/nazimaraz/TeXRender/blob/v0.1.0/docs/known-limitations.md
for the full boundary.

## Validation

This release is designed to be verified before publishing with:

```sh
scripts/verify-release.sh
TEXRENDER_FRESH_CLONE_FULL=1 scripts/verify-fresh-clone.sh
scripts/verify-release-archive.sh
scripts/verify-cpack.sh
scripts/verify-external-consumers.sh
scripts/verify-installed-headers.sh
scripts/verify-backend-packages.sh
scripts/verify-shared-library.sh
scripts/verify-path-with-spaces.sh
scripts/verify-pkg-config.sh
scripts/check-registry-readiness.py
scripts/check-third-party-notices.py
scripts/check-benchmark-baseline.py --input benchmark-results.txt --warn-only
```

CI also covers Linux, macOS, Windows, ASan/UBSan, PR/scheduled
ThreadSanitizer smoke, nightly fuzzing, CodeQL, clang-tidy, Doxygen API
generation, third-party notice drift, coverage, package-manager validation,
WebAssembly, workflow linting, and release smoke tests.
