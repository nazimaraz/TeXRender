#pragma once

#include <cstddef>
#include <cstdint>
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
        struct FaceEntry
        {
            FT_FaceRec_* face{};
            uint32_t id = 0;
        };

        struct CachedGlyph
        {
            Glyph glyph;
            std::list<uint64_t>::iterator lru_it;
        };

        FT_LibraryRec_* library_{};
        std::unordered_map<std::string, FaceEntry> faces_;
        std::unordered_map<uint32_t, FT_FaceRec_*> faces_by_id_;
        std::unordered_map<uint64_t, CachedGlyph> glyphs_;
        std::list<uint64_t> lru_;
        uint32_t next_face_id_ = 1;
    };

    namespace detail
    {
        [[nodiscard]] GlyphCache& shared_cache();
        void set_default_font_path(const std::string& path);
        [[nodiscard]] const std::string& default_font_path();

        [[nodiscard]] bool debug_enabled();
        void debug_log(const std::string& message);
    } // namespace detail
} // namespace TexGui
