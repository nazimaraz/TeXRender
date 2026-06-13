# Support Matrix

This is the support promise for the `0.1.x` line. texgui is pre-1.0, so source
compatibility matters more than ABI stability; see
[`api-compatibility.md`](api-compatibility.md).

## Toolchains

| Platform | Compiler | CI coverage | Notes |
| --- | --- | --- | --- |
| Linux | GCC 14 | yes | Headless, SVG, raylib, SDL2, sanitizers, fuzz smoke, package consumers |
| Linux | Clang 22 | yes | Fuzz target and clang-format/clang-tidy tooling |
| GitHub CodeQL | C/C++ analysis | yes | Manual headless CMake build for semantic analysis |
| macOS | Apple Clang | yes | raylib and headless builds |
| Windows | MSVC | yes | headless static/shared builds and installed consumer smoke |

texgui itself uses C++23 internally. Public headers compile as C++17 and expose
`texgui::texgui` with `cxx_std_17`.

## Backends

| Backend | Status | Primary use |
| --- | --- | --- |
| raylib | supported | Native/windowed demos and WebAssembly live editor |
| SDL2 | supported | Embedding formulas into an SDL renderer |
| headless | supported | CI, services, PNG/PPM output, package tests |
| SVG | supported | Self-contained SVG files and docs/gallery output |
| custom `IDrawSurface` | supported interface | Engine-specific renderer integrations |

`HeadlessSurface` and `SvgSurface` are available in every build. `TEXGUI_BACKEND`
selects the default surface created by `LatexDocument`.

## Dependency Modes

| Mode | Status |
| --- | --- |
| `FetchContent` / `add_subdirectory` | supported and tested |
| installed `find_package(texgui CONFIG REQUIRED)` | supported and tested |
| shared library | supported for headless in CI |
| Conan recipe | starter package metadata, headless/SVG/SDL2 validated in CI |
| vcpkg manifest/overlay | starter metadata with overlay smoke test |
| public package registries | planned after first public tag |

## Runtime Notes

- MicroTeX resources must be shipped with the application; use
  `texgui_copy_resources(target)` in CMake consumers.
- GUI surfaces borrow their graphics context. Destroy formulas/documents before
  closing raylib windows or SDL renderers.
- `try_render()` applies guardrails for user-provided formulas but is not a
  sandbox for hostile workloads.
