#include "ft_text_layout.hpp"
#include "core/document_state.hpp"
#include "glyph_cache.hpp"
#include "raster_size.hpp"
#include <mutex>
#include <stdexcept>

using namespace TeXRender;

FtTextLayout::FtTextLayout(std::wstring text, tex::sptr<FtFont> font)
    : text_{std::move(text)}
    , font_{std::move(font)}
{}

void FtTextLayout::getBounds(tex::Rect& bounds)
{
    const auto lock = std::scoped_lock{detail::render_mutex()};
    auto& cache = detail::shared_cache();
    const auto pixel_size = detail::safe_raster_pixel_size(font_->getSize());
    if (pixel_size <= 0)
    {
        bounds.x = 0.f;
        bounds.y = 0.f;
        bounds.w = 0.f;
        bounds.h = 0.f;
        return;
    }

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
    auto ft = std::dynamic_pointer_cast<FtFont>(font);
    if (!ft)
        throw std::invalid_argument("texrender: TextLayout requires a texrender FreeType font");

    return sptrOf<FtTextLayout>(src, ft);
}
