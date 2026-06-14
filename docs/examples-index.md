# Examples Index

Use this index to pick the smallest example that matches the way you want to
embed texrender.

| Example | Build mode | What it shows |
| --- | --- | --- |
| [`examples/minimal.cpp`](../examples/minimal.cpp) | default raylib | The shortest windowed formula render loop. |
| [`examples/demo_main.cpp`](../examples/demo_main.cpp) | default raylib | Interactive sample browser with text-size and formula changes. |
| [`examples/realtime_values.cpp`](../examples/realtime_values.cpp) | default raylib | Showing signal variables and symbolic formulas, then substituting live values into TeX. |
| [`examples/live_edit.cpp`](../examples/live_edit.cpp) | raylib or WebAssembly | Live formula editing, including the browser demo build. |
| [`examples/raylib_overlay.cpp`](../examples/raylib_overlay.cpp) | default raylib | Drawing formulas as one layer inside an existing raylib scene. |
| [`examples/sdl2_minimal.cpp`](../examples/sdl2_minimal.cpp) | `TEXRENDER_BACKEND=sdl2` | Injecting texrender into a caller-owned `SDL_Renderer`. |
| [`examples/offscreen.cpp`](../examples/offscreen.cpp) | any backend | Rendering one formula to PNG/PPM with `HeadlessSurface`. |
| [`examples/headless_batch.cpp`](../examples/headless_batch.cpp) | any backend | Command-line batch PNG rendering without a window. |
| [`examples/svg_export.cpp`](../examples/svg_export.cpp) | any backend | Writing a self-contained SVG file. |
| [`examples/custom_surface.cpp`](../examples/custom_surface.cpp) | any backend | Implementing a custom `IDrawSurface`. |
| [`examples/formula_label_widget.cpp`](../examples/formula_label_widget.cpp) | any backend | Wrapping texrender in a reusable app/widget object. |
| [`examples/consumer-fetchcontent`](../examples/consumer-fetchcontent) | external CMake | Consuming texrender with `FetchContent`. |
| [`examples/consumer-find-package`](../examples/consumer-find-package) | external CMake | Consuming an installed `find_package(texrender CONFIG REQUIRED)` package. |
| [`examples/consumer-pkg-config`](../examples/consumer-pkg-config) | external Makefile | Consuming an installed package through `pkg-config`. |

## Picking A Starting Point

- Building a game or GUI overlay: start with `minimal.cpp`, then compare
  `realtime_values.cpp`, `raylib_overlay.cpp`, or `sdl2_minimal.cpp`.
- Generating images on a server or in CI: start with `offscreen.cpp` or
  `headless_batch.cpp`.
- Exporting documents or reports: start with `svg_export.cpp`.
- Integrating a custom renderer, engine command buffer, or test double: start
  with `custom_surface.cpp`, then read
  [`docs/backend-authoring.md`](backend-authoring.md).
- Wrapping formulas inside your own UI/application objects: start with
  `formula_label_widget.cpp`.
- Adding texrender to another project: start with the FetchContent, installed
  package, or pkg-config consumer directories.

The longer command walk-throughs live in
[`docs/integration-examples.md`](integration-examples.md), and copyable
application patterns live in [`docs/recipes.md`](recipes.md).
