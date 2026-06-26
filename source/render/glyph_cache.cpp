#include "glyph_cache.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <mutex>
#include <string>
#include <string_view>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "raster_size.hpp"
#include "texrender/cache_options.hpp"

using namespace TeXRender;

namespace
{
    [[nodiscard]] std::string& mutable_default_font_path()
    {
        static auto path = std::string{};
        return path;
    }

    [[nodiscard]] std::mutex& default_font_path_mutex()
    {
        static auto mutex = std::mutex{};
        return mutex;
    }

    void decode_glyph_bitmap(GlyphCache::Glyph& g, const FT_GlyphSlotRec_& slot)
    {
        const auto& bmp = slot.bitmap;
        if (bmp.width > static_cast<unsigned int>(std::numeric_limits<int>::max()) ||
            bmp.rows > static_cast<unsigned int>(std::numeric_limits<int>::max()) || bmp.pitch == std::numeric_limits<int>::min())
        {
            detail::debug_log("FreeType returned an unsupported glyph bitmap size");
            return;
        }

        g.width = static_cast<int>(bmp.width);
        g.height = static_cast<int>(bmp.rows);
        g.pitch = bmp.pitch < 0 ? -bmp.pitch : bmp.pitch;
        g.left = slot.bitmap_left;
        g.top = slot.bitmap_top;
        g.advance = static_cast<float>(slot.advance.x) / 64.f;
        if (g.width <= 0 || g.height <= 0 || g.pitch < g.width || !bmp.buffer)
            return;

        const auto pitch = static_cast<std::size_t>(g.pitch);
        const auto height = static_cast<std::size_t>(g.height);
        if (height > g.pixels.max_size() / pitch)
            return;

        g.pixels.resize(pitch * height);
        const auto* const top_row = bmp.pitch < 0 ? bmp.buffer + static_cast<std::ptrdiff_t>(g.height - 1) * g.pitch : bmp.buffer;
        for (auto row = 0; row < g.height; ++row)
        {
            const auto* const src = top_row + static_cast<std::ptrdiff_t>(row) * bmp.pitch;
            std::copy_n(src, g.pitch, g.pixels.begin() + static_cast<std::ptrdiff_t>(row) * g.pitch);
        }
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

std::uint32_t GlyphCache::face_for(const std::string& path)
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

std::uint32_t GlyphCache::glyph_index_for(const std::uint32_t face_id, const char32_t codepoint) const
{
    const auto face_it = faces_by_id_.find(face_id);
    if (face_it == faces_by_id_.end())
        return 0;

    return FT_Get_Char_Index(face_it->second, codepoint);
}

const GlyphCache::Glyph* GlyphCache::glyph(const std::uint32_t face_id, const char32_t codepoint, const int pixel_size)
{
    return load_glyph(face_id, codepoint, pixel_size, GlyphLoadMode::codepoint);
}

const GlyphCache::Glyph* GlyphCache::glyph_index(const std::uint32_t face_id, const std::uint32_t glyph_index, const int pixel_size)
{
    return load_glyph(face_id, glyph_index, pixel_size, GlyphLoadMode::glyph_index);
}

const GlyphCache::Glyph* GlyphCache::load_glyph(const std::uint32_t face_id, const std::uint32_t glyph_id, const int pixel_size,
    const GlyphLoadMode mode)
{
    if (face_id == 0 || pixel_size <= 0 || pixel_size > detail::max_raster_pixel_size)
        return nullptr;

    const auto face_it = faces_by_id_.find(face_id);
    if (face_it == faces_by_id_.end())
        return nullptr;

    const auto lookup = GlyphLookup{face_id, glyph_id, mode == GlyphLoadMode::glyph_index, pixel_size};
    auto id_it = glyph_ids_.find(lookup);
    const auto new_id = id_it == glyph_ids_.end();
    if (new_id)
        id_it = glyph_ids_.emplace(lookup, next_glyph_key_++).first;

    const auto key = id_it->second;
    if (const auto cached = glyphs_.find(key); cached != glyphs_.end())
    {
        lru_.splice(lru_.begin(), lru_, cached->second.lru_it); // mark most-recently-used
        return &cached->second.glyph;
    }

    const auto face = face_it->second;
    if (FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(pixel_size)) != 0)
    {
        detail::debug_log("failed to set FreeType pixel size");
        if (new_id)
            glyph_ids_.erase(lookup);

        return nullptr;
    }

    Glyph g;
    g.key = key;
    const auto ft_error = mode == GlyphLoadMode::glyph_index ? FT_Load_Glyph(face, glyph_id, FT_LOAD_RENDER)
                                                             : FT_Load_Char(face, glyph_id, FT_LOAD_RENDER);
    if (ft_error)
        detail::debug_log(std::string{"failed to render glyph "} + std::to_string(glyph_id));
    else
        decode_glyph_bitmap(g, *face->glyph);

    lru_.push_front(key);
    const auto it = glyphs_.emplace(key, CachedGlyph{std::move(g), lookup, lru_.begin()}).first;
    const auto max_glyphs = std::max<std::size_t>(1, cache_limits().max_cpu_glyphs);
    for (auto excess = glyphs_.size(); excess > max_glyphs; --excess)
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
        const auto* const value = std::getenv("TEXRENDER_DEBUG");
        return value && value[0] != '\0';
    }();
    return enabled;
}

void detail::debug_log(const std::string_view message) noexcept
{
    if (!debug_enabled())
        return;

    (void) std::fputs("[texrender] ", stderr);
    (void) std::fwrite(message.data(), 1, message.size(), stderr);
    (void) std::fputc('\n', stderr);
}
