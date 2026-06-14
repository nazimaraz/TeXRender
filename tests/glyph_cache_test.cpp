#include <cstdio>
#include <string>
#include "render/glyph_cache.hpp"

#ifndef TEXRENDER_RES_DIR
#define TEXRENDER_RES_DIR "res"
#endif

using TeXRender::GlyphCache;

namespace
{
    int g_failures = 0;

    void check(const bool ok, const char* const expr)
    {
        if (!ok)
        {
            std::printf("FAIL: %s\n", expr);
            ++g_failures;
        }
    }

    std::string font_path()
    {
        return std::string{TEXRENDER_RES_DIR} + "/fonts/latin/cmr10.ttf";
    }
} // namespace

#define CHECK(cond) check((cond), #cond)

int main()
{
    auto cache = GlyphCache{};
    const auto face = cache.face_for(font_path());
    CHECK(face != 0);
    CHECK(cache.face_for(font_path()) == face);
    CHECK(cache.face_for("/no/such/font.ttf") == 0);
    CHECK(cache.face_for("/no/such/font.ttf") == 0); // negative cache, no retry/crash
    const auto* const a1 = cache.glyph(face, U'A', 40);
    CHECK(a1 != nullptr);
    CHECK(a1 != nullptr && a1->width > 0 && a1->height > 0);
    CHECK(a1 != nullptr && !a1->pixels.empty());
    CHECK(a1 != nullptr && a1->advance > 0.f);
    const auto* const a2 = cache.glyph(face, U'A', 40);
    CHECK(a1 == a2); // cache hit returns the same object

    // Different pixel sizes are distinct cache entries and rasterise differently.
    const auto* const big = cache.glyph(face, U'A', 80);
    CHECK(big != nullptr && a1 != nullptr && big->height > a1->height);

    // Guard rails: face_id 0 and non-positive sizes yield null.
    CHECK(cache.glyph(0, U'A', 40) == nullptr);
    CHECK(cache.glyph(face, U'A', 0) == nullptr);

    // The cache survives churn far beyond its capacity (LRU eviction) without
    // invalidating a freshly-inserted glyph.
    for (auto size = 1; size <= 1200; ++size)
        (void) cache.glyph(face, U'x', size);

    const auto* const fresh = cache.glyph(face, U'Z', 41);
    CHECK(fresh != nullptr && !fresh->pixels.empty());

    if (g_failures == 0)
        std::printf("all glyph_cache tests passed\n");

    return g_failures == 0 ? 0 : 1;
}
