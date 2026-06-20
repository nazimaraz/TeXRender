# Dependency And License Audit

This is a maintainer checklist for what texrender builds with and what a release
redistributes. It is not legal advice; use it to keep notices and release assets
honest before publishing.

## Dependency Inventory

| Component | How it is used | How it is resolved | Shipped in install archives? | License notes |
| --- | --- | --- | --- | --- |
| texrender integration code | Public C++ API, backends, examples, tests and CMake package | This repository | Yes | MIT, from [`LICENSE`](../LICENSE). |
| MicroTeX core | LaTeX parsing, box layout, font/resource loading | Pinned `FetchContent` source | Yes, compiled into `libmicrotex_core` | MIT upstream. Keep upstream notices when redistributing. |
| MicroTeX `res/` tree | Runtime fonts, metrics, maps and parser resources | From the pinned MicroTeX checkout | Yes, installed to `share/texrender/res` | Contains several font/resource licenses. Preserve files under `res/fonts/licences/` and per-language `LICENSE` files. |
| FreeType | Glyph rasterisation from bundled TeX fonts | `find_package(Freetype)` or `FetchContent` fallback | Not vendored as source in install archives; linked by builds | Dual FTL/GPLv2. Prefer FTL for permissive projects and keep attribution. |
| tinyxml2 | MicroTeX XML resource parsing | `find_package(tinyxml2)` or `FetchContent` fallback | Not vendored as source in install archives; linked by builds | zlib license. |
| raylib | Optional windowed GPU backend | `find_package(raylib)` or `FetchContent` fallback | Only linked when building the raylib backend | zlib/libpng license. |
| SDL2 | Optional windowed GPU backend | `find_package(SDL2)` or `FetchContent` fallback | Only linked when building the SDL2 backend | zlib license. |

## Release Checks

- Keep [`THIRD_PARTY.md`](../THIRD_PARTY.md) aligned with this inventory.
- Run `python3 scripts/check-third-party-notices.py` before tagging; CI runs the
  same check in the docs and registry-readiness jobs.
- Preserve MicroTeX resource license files in installed archives.
- Verify `cmake --install` puts resources under `share/texrender/res`.
- Verify CPack/source archives do not include local build products.
- For binary archives, document whether they are static or shared and which
  backend they were built with.
- Keep [`docs/upstream-microtex-patch.md`](upstream-microtex-patch.md) aligned
  with the local MicroTeX patch in `cmake/Dependencies.cmake`, and remove the
  patch only after the pinned upstream revision includes the fix.
- For package-manager submissions, follow each registry's license field and
  notice requirements rather than relying only on this repository's MIT license.

## Current Redistribution Shape

The default release archives are headless install archives plus CPack
source/binary archives. They include texrender headers, texrender and MicroTeX static
libraries, CMake package files, pkg-config metadata, and the MicroTeX resource
tree. GUI backends may add raylib or SDL2 linkage depending on the selected
build option, but the release workflow's hand-named install archives are
headless.
