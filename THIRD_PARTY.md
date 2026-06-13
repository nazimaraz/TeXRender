# Third-party licenses

latex-renderer (the integration code in this repository) is MIT-licensed — see
[LICENSE](LICENSE). It builds on the following third-party projects, fetched at
configure time via CMake `FetchContent`. Their licenses live in their respective
upstream repositories. **If you redistribute a binary built with latex-renderer
(together with the MicroTeX `res/` directory), you redistribute these too**, so
keep their notices intact.

| Component | Use | License |
|-----------|-----|---------|
| [MicroTeX](https://github.com/NanoMichael/MicroTeX) | LaTeX parsing & box layout | MIT |
| MicroTeX `res/` fonts | bundled TeX fonts (Computer Modern et al.) | Knuth License, SIL Open Font License (OFL), and the `dsrom` license — see `res/fonts/licences/` in the MicroTeX checkout |
| [raylib](https://www.raylib.com/) | GUI / render backend | zlib/libpng |
| [SDL2](https://www.libsdl.org/) | optional GUI / render backend | zlib |
| [FreeType](https://freetype.org/) | glyph rasterisation | FTL **or** GPLv2 (dual-licensed) |
| [tinyxml2](https://github.com/leethomason/tinyxml2) | MicroTeX resource parsing | zlib |

All of these are permissive. The one to be deliberate about is **FreeType**,
which is dual-licensed under the FreeType License (BSD-style, requires
attribution) or GPLv2 — choose whichever fits your project's licensing.

For the release redistribution checklist, see
[`docs/dependency-license-audit.md`](docs/dependency-license-audit.md).
For the local MicroTeX safety patch carried by this repository, see
[`docs/upstream-microtex-patch.md`](docs/upstream-microtex-patch.md).
