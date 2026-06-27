#include "graphics2d_adapter.hpp"
#include <algorithm>
#include "ft_font.hpp"
#include "raster_size.hpp"

using namespace TeXRender;

Graphics2DAdapter::Graphics2DAdapter(IDrawSurface& surface)
    : surface_{surface}
    , cache_{detail::shared_cache()}
{}

void Graphics2DAdapter::setColor(const microtex::color c)
{
    color_ = c;
    surface_.set_color(c);
}

microtex::color Graphics2DAdapter::getColor() const
{
    return color_;
}

void Graphics2DAdapter::setStroke(const microtex::Stroke& s)
{
    stroke_ = s;
}

const microtex::Stroke& Graphics2DAdapter::getStroke() const
{
    return stroke_;
}

void Graphics2DAdapter::setStrokeWidth(const float w)
{
    stroke_.lineWidth = w;
}

void Graphics2DAdapter::setDash(const std::vector<float>& dash)
{
    dash_ = dash;
}

std::vector<float> Graphics2DAdapter::getDash()
{
    return dash_;
}

microtex::sptr<microtex::Font> Graphics2DAdapter::getFont() const
{
    return font_;
}

void Graphics2DAdapter::setFont(const microtex::sptr<microtex::Font>& font)
{
    font_ = font;
}

float Graphics2DAdapter::getFontSize() const
{
    return font_size_;
}

void Graphics2DAdapter::setFontSize(const float size)
{
    font_size_ = size;
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

const GlyphCache::Glyph* Graphics2DAdapter::raster(const microtex::u16 glyph_index) const
{
    const auto font = std::dynamic_pointer_cast<FtFont>(font_);
    if (!font)
        return nullptr;

    const auto pixel_size = detail::safe_raster_pixel_size(font_size_, transform_.scale_y());
    if (pixel_size <= 0)
        return nullptr;

    const auto face_id = cache_.face_for(font->path());
    return cache_.glyph_index(face_id, glyph_index, pixel_size);
}

void Graphics2DAdapter::drawGlyph(const microtex::u16 glyph, const float x, const float y)
{
    const auto lock = std::scoped_lock{detail::glyph_cache_mutex()};
    const auto* const g = raster(glyph);
    if (!g || g->pixels.empty())
        return;

    const auto bitmap = GlyphBitmap{g->pixels.data(), g->width, g->height, g->pitch, g->left, g->top, g->key};
    surface_.draw_glyph(bitmap, transform_.apply_x(x, y), transform_.apply_y(x, y), transform_.rotation());
}

bool Graphics2DAdapter::beginPath(microtex::i32 /*id*/)
{
    return false;
}

void Graphics2DAdapter::moveTo(float /*x*/, float /*y*/)
{}

void Graphics2DAdapter::lineTo(float /*x*/, float /*y*/)
{}

void Graphics2DAdapter::cubicTo(float /*x1*/, float /*y1*/, float /*x2*/, float /*y2*/, float /*x3*/, float /*y3*/)
{}

void Graphics2DAdapter::quadTo(float /*x1*/, float /*y1*/, float /*x2*/, float /*y2*/)
{}

void Graphics2DAdapter::closePath()
{}

void Graphics2DAdapter::fillPath(microtex::i32 /*id*/)
{
    detail::debug_log("MicroTeX requested path rendering, but texrender is configured for font glyph rendering");
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
