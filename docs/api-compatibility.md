# API Compatibility

texgui is pre-1.0, so the project is still allowed to evolve. The goal is to
make that evolution explicit rather than surprising consumers.

## Public Surface

These files are the public API:

- `include/texgui/*.hpp`
- the exported CMake target `texgui::texgui`
- CMake helper functions installed from `cmake/texguiHelpers.cmake`
- installed `pkg-config` metadata from `texgui.pc`
- documented executable examples in `examples/`

Implementation files under `source/`, bundled MicroTeX integration details, test
helpers, and CI scripts are not API.

## Naming

The repository and package-manager package name is `latex-renderer`. The public
C++ namespace, installed headers, CMake package, exported target, and pkg-config
file use `texgui`/`TexGui`: `TexGui::LatexDocument`,
`#include <texgui/texgui.hpp>`, `find_package(texgui CONFIG REQUIRED)`,
`texgui::texgui`, and `texgui.pc`.

## Compatibility Promise

For `0.1.x` releases:

- Public headers stay C++17-compatible.
- `texgui::texgui` remains the CMake target name.
- `texgui_copy_resources(target)` remains the resource-copy helper.
- Existing public functions and classes are not renamed or removed without a
  changelog entry and migration note.
- New backends may be added without changing `IDrawSurface`.

Before `1.0`, signatures may still change when they fix an ownership, safety or
dependency problem. Those changes must be called out in `CHANGELOG.md` and in
release notes.

## ABI

texgui does not promise binary ABI stability yet. Consumers should rebuild when
upgrading the library, especially across compiler versions, standard libraries,
or shared-library builds. Source compatibility is the priority until a stable
`1.x` line exists.

## Guardrails

The repository keeps several compatibility checks in CI:

- public headers compile as C++17
- the public API snapshot in `docs/public-api-snapshot.txt` matches the current
  headers and install metadata
- installed `find_package(texgui CONFIG REQUIRED)` consumers build and run
- installed `pkg-config --static texgui` consumers build and run on Unix-like
  hosts with pkg-config metadata for dependencies
- shared-library install/consumer checks run on Windows
- Conan and vcpkg entry points build package-shaped consumers

Any public API change should update the relevant docs, examples, and tests in
the same pull request.
