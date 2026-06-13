#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

struct FT_LibraryRec_;
struct FT_FaceRec_;

namespace TexGui
{
    class GlyphCache
    {
    public:
        struct Glyph
        {
            std::vector<unsigned char> pixels;
            int width = 0;
            int height = 0;
            int pitch = 0;
            int left = 0;
            int top = 0;
            float advance = 0.f;
            uint64_t key = 0;
        };

        GlyphCache();
        GlyphCache(const GlyphCache&) = delete;
        GlyphCache(GlyphCache&&) = delete;
        GlyphCache& operator=(const GlyphCache&) = delete;
        GlyphCache& operator=(GlyphCache&&) = delete;
        ~GlyphCache();

        // Stable id for the font file at path; 0 if it cannot be loaded
        // (failures are cached, so retries are cheap).
        [[nodiscard]] uint32_t face_for(const std::string& path);

        // Rasterise (or fetch) one glyph. The returned pointer is owned by the
        // cache and is only guaranteed valid until the next glyph() call, which
        // may evict the entry — consume it immediately, never store it.
        [[nodiscard]] const Glyph* glyph(uint32_t face_id, char32_t codepoint, int pixel_size);

    private:
        struct GlyphLookup
        {
            uint32_t face_id = 0;
            char32_t codepoint = 0;
            int pixel_size = 0;

            [[nodiscard]] bool operator==(const GlyphLookup& other) const noexcept
            {
                return face_id == other.face_id && codepoint == other.codepoint && pixel_size == other.pixel_size;
            }
        };

        struct GlyphLookupHash
        {
            [[nodiscard]] std::size_t operator()(const GlyphLookup& lookup) const noexcept
            {
                auto seed = std::hash<uint32_t>{}(lookup.face_id);
                seed ^= std::hash<uint32_t>{}(static_cast<uint32_t>(lookup.codepoint)) + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
                seed ^= std::hash<int>{}(lookup.pixel_size) + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
                return seed;
            }
        };

        struct FaceEntry
        {
            FT_FaceRec_* face{};
            uint32_t id = 0;
        };

        struct CachedGlyph
        {
            Glyph glyph;
            GlyphLookup lookup;
            std::list<uint64_t>::iterator lru_it;
        };

        FT_LibraryRec_* library_{};
        std::unordered_map<std::string, FaceEntry> faces_;
        std::unordered_map<uint32_t, FT_FaceRec_*> faces_by_id_;
        std::unordered_map<uint64_t, CachedGlyph> glyphs_;
        std::unordered_map<GlyphLookup, uint64_t, GlyphLookupHash> glyph_ids_;
        std::list<uint64_t> lru_;
        uint32_t next_face_id_ = 1;
        uint64_t next_glyph_key_ = 1;
    };

    namespace detail
    {
        [[nodiscard]] GlyphCache& shared_cache();
        void set_default_font_path(const std::string& path);
        [[nodiscard]] std::string default_font_path();

        [[nodiscard]] bool debug_enabled();
        void debug_log(const std::string& message);
    } // namespace detail
} // namespace TexGui
