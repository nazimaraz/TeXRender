#pragma once

// Hand-written so it is browsable in the repo; CMake fails the configure if
// these drift from project(VERSION) in CMakeLists.txt.
#define TEXGUI_VERSION_MAJOR 0
#define TEXGUI_VERSION_MINOR 1
#define TEXGUI_VERSION_PATCH 0

namespace TexGui
{
    inline constexpr int version_major = TEXGUI_VERSION_MAJOR;
    inline constexpr int version_minor = TEXGUI_VERSION_MINOR;
    inline constexpr int version_patch = TEXGUI_VERSION_PATCH;
} // namespace TexGui
