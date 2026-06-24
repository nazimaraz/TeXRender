#pragma once

/// Convenience umbrella header for the stable, backend-neutral texrender API.
///
/// Backend-specific surfaces stay opt-in:
/// include `texrender/raylib_surface.hpp` or `texrender/sdl2_surface.hpp` only when
/// the package was built with that backend.
#include "texrender/cache_options.hpp"
#include "texrender/draw_surface.hpp"
#include "texrender/formula_layout.hpp"
#include "texrender/glyph_bitmap.hpp"
#include "texrender/headless_surface.hpp"
#include "texrender/latex_document.hpp"
#include "texrender/render_handle.hpp"
#include "texrender/svg_surface.hpp"
#include "texrender/version.hpp"
