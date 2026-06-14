#pragma once

// Hand-written so it is browsable in the repo; CMake fails the configure if
// these drift from project(VERSION) in CMakeLists.txt.
#define TEXRENDER_VERSION_MAJOR 0
#define TEXRENDER_VERSION_MINOR 1
#define TEXRENDER_VERSION_PATCH 0

namespace TeXRender
{
    /// Compile-time semantic version constants for feature checks and logging.
    ///
    /// These values match `project(VERSION)` in CMake and are validated at
    /// configure time.
    ///
    /// The version constants describe the texrender/TeXRender package, not
    /// the pinned MicroTeX revision. For dependency feature checks, prefer
    /// `texrender/config.hpp` and CMake package component metadata.
    /// Major component of the texrender semantic version.
    inline constexpr int version_major = TEXRENDER_VERSION_MAJOR;
    /// Minor component of the texrender semantic version.
    inline constexpr int version_minor = TEXRENDER_VERSION_MINOR;
    /// Patch component of the texrender semantic version.
    inline constexpr int version_patch = TEXRENDER_VERSION_PATCH;
} // namespace TeXRender
