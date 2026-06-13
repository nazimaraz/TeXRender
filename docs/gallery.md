# Gallery

## Live Editor

![texgui live editor](demo.png)

The live editor uses the raylib backend and runs as either a native executable or
WebAssembly.

## Headless Output

![headless formula output](offscreen-example.png)

The headless backend renders into an RGBA buffer and can write PNG/PPM snapshots for
tests, services, and command-line tooling. The PNG above is generated from
`texgui_headless_batch`.

## SVG Export

`texgui_svg_export` writes a self-contained SVG. Shape primitives stay as SVG
elements; glyphs are embedded as small PNG data URIs so the file has no runtime
font dependency.

## Generated Artifacts

The gallery sample set is listed in [`gallery-formulas.txt`](gallery-formulas.txt).
After building a backend that includes `texgui_offscreen` and
`texgui_svg_export`, regenerate PNG/SVG artifacts with:

```sh
TEXGUI_BUILD_DIR=build-headless scripts/generate-gallery.sh
```

CI uploads these generated artifacts from the Linux headless job, and
`scripts/verify-release.sh` renders them during local release verification.
