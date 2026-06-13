# Gallery

## Live Editor

![texgui live editor](demo.png)

The live editor uses the raylib backend and runs as either a native executable or
WebAssembly.

## Headless Output

![headless formula output](offscreen-example.png)

The headless backend renders into an RGBA buffer and can write PPM snapshots for
tests, services, and command-line tooling. The PNG above is generated from
`texgui_headless_batch`.
