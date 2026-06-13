#include "render/glyph_cache.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "texgui/cache_options.hpp"

using namespace TexGui;

namespace
{
    std::string& mutable_default_font_path()
    {
        static auto path = std::string{};
        return path;
    }

    std::mutex& default_font_path_mutex()
    {
        static auto mutex = std::mutex{};
        return mutex;
    }
} // namespace

GlyphCache::GlyphCache()
{
    if (FT_Init_FreeType(&library_) != 0)
    {
        library_ = nullptr; // face_for() then yields 0 and glyphs stay null
        detail::debug_log("failed to initialise FreeType; no glyph will render");
    }
}

GlyphCache::~GlyphCache()
{
    for (auto& entry : faces_)
    {
        if (!entry.second.face)
            continue;

        FT_Done_Face(entry.second.face);
    }

    if (!library_)
        return;

    FT_Done_FreeType(library_);
}

uint32_t GlyphCache::face_for(const std::string& path)
{
    if (path.empty() || !library_)
        return 0;

    if (const auto found = faces_.find(path); found != faces_.end())
        return found->second.id;

    auto face = FT_Face{};
    if (FT_New_Face(library_, path.c_str(), 0, &face) != 0)
    {
        detail::debug_log("failed to load font file: " + path);
        faces_[path] = FaceEntry{nullptr, 0};
        return 0;
    }

    const auto id = next_face_id_++;
    faces_[path] = FaceEntry{face, id};
    faces_by_id_[id] = face;
    return id;
}

const GlyphCache::Glyph* GlyphCache::glyph(const uint32_t face_id, const char32_t codepoint, const int pixel_size)
{
    if (face_id == 0 || pixel_size <= 0)
        return nullptr;

    const auto lookup = GlyphLookup{face_id, codepoint, pixel_size};
    auto id_it = glyph_ids_.find(lookup);
    if (id_it == glyph_ids_.end())
        id_it = glyph_ids_.emplace(lookup, next_glyph_key_++).first;

    const auto key = id_it->second;
    if (const auto cached = glyphs_.find(key); cached != glyphs_.end())
    {
        lru_.splice(lru_.begin(), lru_, cached->second.lru_it); // mark most-recently-used
        return &cached->second.glyph;
    }

    const auto face_it = faces_by_id_.find(face_id);
    if (face_it == faces_by_id_.end())
        return nullptr;

    const auto face = face_it->second;
    FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(pixel_size));

    Glyph g;
    g.key = key;
    if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER) != 0)
    {
        auto message = std::array<char, 64>{};
        std::snprintf(message.data(), message.size(), "failed to render glyph U+%04X", static_cast<uint32_t>(codepoint));
        detail::debug_log(message.data());
    }
    else
    {
        const auto slot = face->glyph;
        const auto& bmp = slot->bitmap;
        g.width = static_cast<int>(bmp.width);
        g.height = static_cast<int>(bmp.rows);
        g.pitch = bmp.pitch < 0 ? -bmp.pitch : bmp.pitch;
        g.left = slot->bitmap_left;
        g.top = slot->bitmap_top;
        g.advance = static_cast<float>(slot->advance.x) / 64.f;
        if (g.width > 0 && g.height > 0 && bmp.buffer)
        {
            g.pixels.resize(static_cast<size_t>(g.pitch) * static_cast<size_t>(g.height));
            // FT_LOAD_RENDER produces down-flow bitmaps (pitch > 0) today, but
            // handle up-flow too: there the top image row sits at the end of
            // the buffer and the negative bmp.pitch walks backwards from it.
            const auto* const top_row =
                bmp.pitch < 0 ? bmp.buffer + static_cast<std::ptrdiff_t>(g.height - 1) * g.pitch : bmp.buffer;
            for (auto row = 0; row < g.height; ++row)
            {
                const auto* const src = top_row + static_cast<std::ptrdiff_t>(row) * bmp.pitch;
                std::copy_n(src, g.pitch, g.pixels.begin() + static_cast<std::ptrdiff_t>(row) * g.pitch);
            }
        }
    }

    lru_.push_front(key);
    const auto emplaced = glyphs_.emplace(key, CachedGlyph{std::move(g), lookup, lru_.begin()});
    const auto it = emplaced.first;
    const auto max_glyphs = std::max<size_t>(1, cache_limits().max_cpu_glyphs);
    while (glyphs_.size() > max_glyphs)
    {
        const auto oldest = lru_.back();
        if (const auto old_glyph = glyphs_.find(oldest); old_glyph != glyphs_.end())
            glyph_ids_.erase(old_glyph->second.lookup);
        glyphs_.erase(oldest);
        lru_.pop_back();
    }

    return &it->second.glyph;
}

GlyphCache& detail::shared_cache()
{
    static auto cache = GlyphCache{};
    return cache;
}

void detail::set_default_font_path(const std::string& path)
{
    const auto lock = std::scoped_lock{default_font_path_mutex()};
    mutable_default_font_path() = path;
}

std::string detail::default_font_path()
{
    const auto lock = std::scoped_lock{default_font_path_mutex()};
    return mutable_default_font_path();
}

bool detail::debug_enabled()
{
    static const auto enabled = [] {
        const auto* const value = std::getenv("TEXGUI_DEBUG");
        return value && value[0] != '\0';
    }();
    return enabled;
}

void detail::debug_log(const std::string& message)
{
    if (debug_enabled())
        std::fprintf(stderr, "[texgui] %s\n", message.c_str());
}
