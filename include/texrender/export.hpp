#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(TEXRENDER_BUILDING_LIBRARY)
#define TEXRENDER_EXPORT __declspec(dllexport)
#elif defined(TEXRENDER_SHARED)
#define TEXRENDER_EXPORT __declspec(dllimport)
#else
#define TEXRENDER_EXPORT
#endif
#elif defined(TEXRENDER_BUILDING_LIBRARY)
#define TEXRENDER_EXPORT __attribute__((visibility("default")))
#else
#define TEXRENDER_EXPORT
#endif
