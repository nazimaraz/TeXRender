#include "render/ft_text_layout.hpp"
#include "render/glyph_cache.hpp"

using namespace TexGui;

FtTextLayout::FtTextLayout(std::wstring text, tex::sptr<FtFont> font)
    : text_{std::move(text)}
    , font_{std::move(font)}
{}

void FtTextLayout::getBounds(tex::Rect& bounds)
{
    auto& cache = detail::shared_cache();
    const auto pixel_size = std::max(1, static_cast<int>(std::lround(font_->getSize())));
    const auto face_id = cache.face_for(font_->path());

    auto width = 0.f;
    auto ascent = 0;
    auto descent = 0;
    for (const wchar_t wc : text_)
    {
        const auto* const g = cache.glyph(face_id, static_cast<char32_t>(wc), pixel_size);
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

void FtTextLayout::draw(tex::Graphics2D& g2, const float x, const float y)
{
    g2.setFont(font_.get());
    g2.drawText(text_, x, y);
}

tex::sptr<tex::TextLayout> tex::TextLayout::create(const std::wstring& src, const sptr<Font>& font)
{
    auto ft = std::static_pointer_cast<FtFont>(font);
    return sptrOf<FtTextLayout>(src, ft);
}
