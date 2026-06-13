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

The committed gallery sample set is listed in
[`gallery-formulas.txt`](gallery-formulas.txt), with generated PNG/SVG output in
[`docs/gallery/`](gallery/).

| Formula | PNG | SVG |
| --- | --- | --- |
| Pythagorean | ![Pythagorean theorem](gallery/pythagorean.png) | [SVG](gallery/pythagorean.svg) |
| Fraction | ![Fraction formula](gallery/fraction.png) | [SVG](gallery/fraction.svg) |
| Radical | ![Radical formula](gallery/radical.png) | [SVG](gallery/radical.svg) |
| Integral | ![Integral formula](gallery/integral.png) | [SVG](gallery/integral.svg) |
| Matrix | ![Matrix formula](gallery/matrix.png) | [SVG](gallery/matrix.svg) |
| Colored | ![Colored formula](gallery/colored.png) | [SVG](gallery/colored.svg) |

After building a backend that includes `texgui_offscreen` and
`texgui_svg_export`, refresh the committed PNG/SVG artifacts with:

```sh
TEXGUI_BUILD_DIR=build-headless scripts/generate-gallery.sh docs/gallery-formulas.txt docs/gallery
scripts/generate-preview-gif.sh
```

CI also uploads generated gallery artifacts from the Linux headless job, and
`scripts/verify-release.sh` renders them during local release verification.
