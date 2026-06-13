#pragma once

/// Convenience umbrella header for the stable, backend-neutral texgui API.
///
/// Backend-specific surfaces stay opt-in:
/// include `texgui/raylib_surface.hpp` or `texgui/sdl2_surface.hpp` only when
/// the package was built with that backend.
#include "texgui/cache_options.hpp"
#include "texgui/draw_surface.hpp"
#include "texgui/formula_layout.hpp"
#include "texgui/glyph_bitmap.hpp"
#include "texgui/headless_surface.hpp"
#include "texgui/latex_document.hpp"
#include "texgui/render_handle.hpp"
#include "texgui/svg_surface.hpp"
#include "texgui/version.hpp"
