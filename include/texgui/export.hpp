#pragma once

// Symbol visibility for shared-library builds. Static builds leave the macro
// empty; consumers of a Windows DLL receive dllimport via TEXGUI_SHARED.
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(TEXGUI_BUILDING_LIBRARY)
#define TEXGUI_EXPORT __declspec(dllexport)
#elif defined(TEXGUI_SHARED)
#define TEXGUI_EXPORT __declspec(dllimport)
#else
#define TEXGUI_EXPORT
#endif
#elif defined(TEXGUI_BUILDING_LIBRARY)
#define TEXGUI_EXPORT __attribute__((visibility("default")))
#else
#define TEXGUI_EXPORT
#endif
