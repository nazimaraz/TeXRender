#include "render/graphics2d_adapter.hpp"
#include <algorithm>
#include <cmath>
#include "render/ft_font.hpp"
#include "render/raster_size.hpp"

using namespace TeXRender;

Graphics2DAdapter::Graphics2DAdapter(IDrawSurface& surface)
    : surface_{surface}
    , cache_{detail::shared_cache()}
{}

void Graphics2DAdapter::setColor(const tex::color c)
{
    color_ = c;
    surface_.set_color(c);
}

tex::color Graphics2DAdapter::getColor() const
{
    return color_;
}

void Graphics2DAdapter::setStroke(const tex::Stroke& s)
{
    stroke_ = s;
}

const tex::Stroke& Graphics2DAdapter::getStroke() const
{
    return stroke_;
}

void Graphics2DAdapter::setStrokeWidth(const float w)
{
    stroke_.lineWidth = w;
}

const tex::Font* Graphics2DAdapter::getFont() const
{
    return font_;
}

void Graphics2DAdapter::setFont(const tex::Font* font)
{
    font_ = font;
}

void Graphics2DAdapter::translate(const float dx, const float dy)
{
    transform_.translate(dx, dy);
}

void Graphics2DAdapter::scale(const float scale_x, const float scale_y)
{
    transform_.scale(scale_x, scale_y);
}

void Graphics2DAdapter::rotate(const float angle)
{
    transform_.rotate(angle);
}

void Graphics2DAdapter::rotate(const float angle, const float px, const float py)
{
    transform_.rotate(angle, px, py);
}

void Graphics2DAdapter::reset()
{
    transform_.reset();
}

float Graphics2DAdapter::sx() const
{
    return transform_.scale_x();
}

float Graphics2DAdapter::sy() const
{
    return transform_.scale_y();
}

const GlyphCache::Glyph* Graphics2DAdapter::raster(const char32_t codepoint) const
{
    const auto* const font = dynamic_cast<const FtFont*>(font_);
    if (!font)
        return nullptr;

    const auto pixel_size = detail::safe_raster_pixel_size(font->getSize(), transform_.scale_y());
    if (pixel_size <= 0)
        return nullptr;

    const auto face_id = cache_.face_for(font->path());
    return cache_.glyph(face_id, codepoint, pixel_size);
}

void Graphics2DAdapter::drawChar(const wchar_t c, const float x, const float y)
{
    const auto* const g = raster(static_cast<char32_t>(c));
    if (!g || g->pixels.empty())
        return;

    const auto bitmap = GlyphBitmap{g->pixels.data(), g->width, g->height, g->pitch, g->left, g->top, g->key};
    surface_.draw_glyph(bitmap, transform_.apply_x(x, y), transform_.apply_y(x, y), transform_.rotation());
}

void Graphics2DAdapter::drawText(const std::wstring& t, const float x, const float y)
{
    const auto angle = transform_.rotation();
    const auto cs = std::cos(angle);
    const auto sn = std::sin(angle);
    auto pen_x = transform_.apply_x(x, y);
    auto pen_y = transform_.apply_y(x, y);

    for (const wchar_t wc : t)
    {
        const auto* const g = raster(static_cast<char32_t>(wc));
        if (!g)
            continue;

        if (!g->pixels.empty())
        {
            const auto bitmap = GlyphBitmap{g->pixels.data(), g->width, g->height, g->pitch, g->left, g->top, g->key};
            surface_.draw_glyph(bitmap, pen_x, pen_y, angle);
        }
        pen_x += g->advance * cs;
        pen_y += g->advance * sn;
    }
}

void Graphics2DAdapter::drawLine(const float x1, const float y1, const float x2, const float y2)
{
    surface_.set_color(color_);
    surface_.draw_line(transform_.apply_x(x1, y1), transform_.apply_y(x1, y1), transform_.apply_x(x2, y2),
        transform_.apply_y(x2, y2), std::max(1.f, stroke_.lineWidth * transform_.scale_y()));
}

void Graphics2DAdapter::drawRect(const float x, const float y, const float w, const float h)
{
    surface_.set_color(color_);
    surface_.draw_rect(transform_.apply_x(x, y), transform_.apply_y(x, y), w * transform_.scale_x(), h * transform_.scale_y(),
        std::max(1.f, stroke_.lineWidth * transform_.scale_y()));
}

void Graphics2DAdapter::fillRect(const float x, const float y, const float w, const float h)
{
    surface_.set_color(color_);
    surface_.fill_rect(transform_.apply_x(x, y), transform_.apply_y(x, y), w * transform_.scale_x(), h * transform_.scale_y());
}

void Graphics2DAdapter::drawRoundRect(const float x, const float y, const float w, const float h, const float rx, const float ry)
{
    surface_.set_color(color_);
    surface_.draw_round_rect(transform_.apply_x(x, y), transform_.apply_y(x, y), w * transform_.scale_x(), h * transform_.scale_y(),
        rx * transform_.scale_x(), ry * transform_.scale_y(), std::max(1.f, stroke_.lineWidth * transform_.scale_y()));
}

void Graphics2DAdapter::fillRoundRect(const float x, const float y, const float w, const float h, const float rx, const float ry)
{
    surface_.set_color(color_);
    surface_.fill_round_rect(transform_.apply_x(x, y), transform_.apply_y(x, y), w * transform_.scale_x(), h * transform_.scale_y(),
        rx * transform_.scale_x(), ry * transform_.scale_y());
}
