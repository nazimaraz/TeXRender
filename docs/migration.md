# Migration Guide

TeXRender is pre-1.0, so public APIs may still evolve. This guide records the
intended migration path for consumers as the library moves through `0.x`.

## Version 0.1.x

`0.1.x` is the first public compatibility line.

Stable source-level entry points:

- `TeXRender::LatexDocument`
- `TeXRender::RenderHandle`
- `TeXRender::RenderOptions`
- `TeXRender::RenderResult` / `TeXRender::RenderError`
- `TeXRender::IDrawSurface`
- `TeXRender::HeadlessSurface`
- `TeXRender::SvgSurface`
- `TeXRender::RaylibSurface`
- `TeXRender::Sdl2Surface`
- `TeXRender::CacheLimits`
- formula layout helpers in `texrender/formula_layout.hpp`
- CMake target `texrender::texrender`
- CMake helper `texrender_copy_resources(target)`

Expected migration cost inside `0.1.x`:

- patch releases should not require consumer source changes for documented APIs
- new overloads, backends, helpers, tests, and docs may be added
- ABI stability is not promised before `1.0`

## Common Upgrades

### From throwing-only rendering to guarded input

Use `render()` for trusted literals:

```cpp
auto formula = document.render(R"(\sqrt{x+1})", 48.f, 0xff000000);
```

Use `try_render()` or `try_render_detailed()` for caller-supplied text:

```cpp
auto result = document.try_render_detailed(user_input, 48.f, 0xff000000);
if (!result)
    show_error(result.error().message);
```

### From top-left layout to baseline layout

Use `texrender/formula_layout.hpp` when formulas need to align with surrounding UI
text:

```cpp
TeXRender::draw_at_baseline(formula, x, row_baseline);
```

### From default GUI backend to injected surfaces

Inject a surface when you need deterministic output, server-side rendering, or a
custom renderer:

```cpp
auto surface = std::make_unique<TeXRender::HeadlessSurface>();
auto& canvas = *surface;
TeXRender::LatexDocument document(TEXRENDER_RES_DIR, std::move(surface));
```

## Before 1.0

Potential breaking changes before `1.0` should be documented in:

- `CHANGELOG.md`
- this migration guide
- `docs/api-compatibility.md`
- release notes for the affected tag

When possible, prefer additive APIs and deprecation windows over abrupt removal.
