#pragma once

// Hand-written so it is browsable in the repo; CMake fails the configure if
// these drift from project(VERSION) in CMakeLists.txt.
#define TEXGUI_VERSION_MAJOR 0
#define TEXGUI_VERSION_MINOR 1
#define TEXGUI_VERSION_PATCH 0

namespace TexGui
{
    /// Compile-time semantic version constants for feature checks and logging.
    ///
    /// These values match `project(VERSION)` in CMake and are validated at
    /// configure time.
    ///
    /// The version constants describe the texgui/latex-renderer package, not
    /// the pinned MicroTeX revision. For dependency feature checks, prefer
    /// `texgui/config.hpp` and CMake package component metadata.
    /// Major component of the texgui semantic version.
    inline constexpr int version_major = TEXGUI_VERSION_MAJOR;
    /// Minor component of the texgui semantic version.
    inline constexpr int version_minor = TEXGUI_VERSION_MINOR;
    /// Patch component of the texgui semantic version.
    inline constexpr int version_patch = TEXGUI_VERSION_PATCH;
} // namespace TexGui
