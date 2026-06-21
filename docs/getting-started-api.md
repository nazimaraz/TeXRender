# Getting Started With The API

This page is the short path from an empty CMake target to a rendered formula.
For the full behavioral contract, read [API Notes](api.md) after the first
render works.

## Install Or Fetch

Installed package:

```cmake
find_package(texrender CONFIG REQUIRED COMPONENTS headless svg)

target_link_libraries(my_app PRIVATE texrender::texrender)
texrender_copy_resources(my_app)
```

`texrender_copy_resources(my_app)` copies MicroTeX resources beside the target
and defines `TEXRENDER_RES_DIR` for that target.

FetchContent:

```cmake
include(FetchContent)

set(TEXRENDER_BACKEND headless CACHE STRING "" FORCE)
set(TEXRENDER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TEXRENDER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TEXRENDER_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    texrender
    GIT_REPOSITORY https://github.com/nazimaraz/TeXRender.git
    GIT_TAG        v0.1.0
)
FetchContent_MakeAvailable(texrender)

target_link_libraries(my_app PRIVATE texrender::texrender)
texrender_copy_resources(my_app)
```

Use `#include <texrender/texrender.hpp>` for the backend-neutral API. Include
backend-specific headers only when you need a concrete surface type.

## Render PNG Offscreen

Use `TeXRender::HeadlessSurface` for tests, services, command-line tools, and CI
jobs that should not create a window.

\snippet examples/offscreen.cpp headless-render

After drawing, write the canvas:

```cpp
canvas.write_png("formula.png");
canvas.write_ppm("formula.ppm");
```

## Render SVG

`TeXRender::SvgSurface` records the same draw calls into a self-contained SVG.
Glyphs are embedded as small PNG data URIs because MicroTeX reaches surfaces
after glyph rasterization.

\snippet examples/svg_export.cpp svg-render

After drawing, write or inspect the SVG:

```cpp
canvas.write_svg("formula.svg");
const auto text = canvas.svg();
```

## Custom Surface

Implement `TeXRender::IDrawSurface` when your application already owns a renderer
or command buffer. TeXRender handles parsing, layout, transforms, and glyph
rasterization; your surface receives immediate drawing calls.

\snippet examples/custom_surface.cpp custom-surface

Then inject it into a document:

```cpp
auto owned = std::make_unique<CountingSurface>();
auto* stats = owned.get();
TeXRender::LatexDocument document(TEXRENDER_RES_DIR, std::move(owned));
auto formula = document.render(R"(\frac{a^2+b^2}{c})", 48.f, 0xff223344);
formula.draw(0, 0);
```

## Input And Lifetime Rules

Use `render()` for trusted formulas. Use `try_render()` or
`try_render_detailed()` for caller-supplied formulas; those APIs apply or accept
`TeXRender::RenderOptions` guardrails and do not throw parser failures through
your UI or request loop.

`TeXRender::RenderHandle` is move-only and keeps the document's internal render
state alive, so handles can outlive the `TeXRender::LatexDocument` that produced
them. With GPU-backed surfaces, destroy formulas and documents before closing the
graphics context so cached textures can release through the correct toolkit.

`TeXRender::shutdown()` is optional and idempotent. It is safe to call while
documents or handles still exist; in that case it does nothing. Normal process
teardown releases the shared glyph cache and MicroTeX state.
