#include "ft_text_layout.hpp"
#include "core/document_state.hpp"
#include "ft_font.hpp"
#include "glyph_cache.hpp"
#include "raster_size.hpp"
#include "utils/utils.h"
#include <limits>
#include <mutex>
#include <vector>

using namespace TeXRender;

namespace
{
    std::vector<char32_t> decode_utf8_lossy(const std::string& text)
    {
        auto out = std::vector<char32_t>{};
        for (auto index = std::size_t{0}; index < text.size();)
        {
            if (const auto byte = static_cast<unsigned char>(text[index]); byte < 0x80u)
            {
                out.push_back(byte);
                ++index;
            }
            else if ((byte & 0xe0u) == 0xc0u && index + 1 < text.size())
            {
                out.push_back(((byte & 0x1fu) << 6u) | (static_cast<unsigned char>(text[index + 1]) & 0x3fu));
                index += 2;
            }
            else if ((byte & 0xf0u) == 0xe0u && index + 2 < text.size())
            {
                out.push_back(((byte & 0x0fu) << 12u) | ((static_cast<unsigned char>(text[index + 1]) & 0x3fu) << 6u) |
                              (static_cast<unsigned char>(text[index + 2]) & 0x3fu));
                index += 3;
            }
            else if ((byte & 0xf8u) == 0xf0u && index + 3 < text.size())
            {
                out.push_back(((byte & 0x07u) << 18u) | ((static_cast<unsigned char>(text[index + 1]) & 0x3fu) << 12u) |
                              ((static_cast<unsigned char>(text[index + 2]) & 0x3fu) << 6u) |
                              (static_cast<unsigned char>(text[index + 3]) & 0x3fu));
                index += 4;
            }
            else
            {
                out.push_back('?');
                ++index;
            }
        }
        return out;
    }

    class FtPlatformFactory final : public microtex::PlatformFactory
    {
    public:
        microtex::sptr<microtex::Font> createFont(const std::string& file) override
        {
            return microtex::sptrOf<FtFont>(file.empty() ? detail::default_font_path() : file);
        }

        microtex::sptr<microtex::TextLayout> createTextLayout(const std::string& src, microtex::FontStyle /*style*/,
            const float size) override
        {
            return microtex::sptrOf<FtTextLayout>(src, detail::default_font_path(), size);
        }
    };
} // namespace

FtTextLayout::FtTextLayout(std::string text, std::string font_path, const float font_size)
    : text_{std::move(text)}
    , font_path_{std::move(font_path)}
    , font_size_{font_size}
{}

void FtTextLayout::getBounds(microtex::Rect& bounds)
{
    const auto lock = std::scoped_lock{detail::render_mutex()};
    auto& cache = detail::shared_cache();
    const auto pixel_size = detail::safe_raster_pixel_size(font_size_);
    if (pixel_size <= 0)
    {
        bounds.x = 0.f;
        bounds.y = 0.f;
        bounds.w = 0.f;
        bounds.h = 0.f;
        return;
    }

    const auto face_id = cache.face_for(font_path_);
    auto width = 0.f;
    auto ascent = 0;
    auto descent = 0;
    for (const auto codepoint : decode_utf8_lossy(text_))
    {
        const auto* const g = cache.glyph(face_id, codepoint, pixel_size);
        if (!g)
            continue;

        width += g->advance;
        ascent = std::max(ascent, g->top);
        descent = std::max(descent, g->height - g->top);
    }

    bounds.x = 0.f;
    bounds.y = static_cast<float>(-ascent);
    bounds.w = width;
    bounds.h = static_cast<float>(ascent + descent);
}

void FtTextLayout::draw(microtex::Graphics2D& g2, const float x, const float y)
{
    g2.setFont(microtex::sptrOf<FtFont>(font_path_));
    g2.setFontSize(font_size_);
    const auto pixel_size = detail::safe_raster_pixel_size(font_size_);
    auto& cache = detail::shared_cache();
    const auto face_id = cache.face_for(font_path_);
    auto pen_x = x;
    for (const auto codepoint : decode_utf8_lossy(text_))
    {
        const auto glyph_index = cache.glyph_index_for(face_id, codepoint);
        if (glyph_index == 0 || glyph_index > std::numeric_limits<microtex::u16>::max())
            continue;

        const auto* const glyph = cache.glyph_index(face_id, glyph_index, pixel_size);
        if (!glyph)
            continue;

        g2.drawGlyph(static_cast<microtex::u16>(glyph_index), pen_x, y);
        pen_x += glyph->advance;
    }
}

void detail::register_microtex_platform_factory()
{
    static auto registered = false;
    if (registered)
        return;

    microtex::PlatformFactory::registerFactory("texrender-freetype", std::make_unique<FtPlatformFactory>());
    microtex::PlatformFactory::activate("texrender-freetype");
    registered = true;
}
