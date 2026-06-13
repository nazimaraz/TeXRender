#include "texgui/headless_surface.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace TexGui;

namespace
{
    [[nodiscard]] unsigned char channel(const uint32_t argb, const unsigned shift)
    {
        return static_cast<unsigned char>((argb >> shift) & 0xffu);
    }
} // namespace

void HeadlessSurface::set_color(const uint32_t argb)
{
    a_ = channel(argb, 24);
    r_ = channel(argb, 16);
    g_ = channel(argb, 8);
    b_ = channel(argb, 0);
}

void HeadlessSurface::resize(const int width, const int height)
{
    width_ = std::max(0, width);
    height_ = std::max(0, height);
    pixels_.assign(static_cast<size_t>(width_) * static_cast<size_t>(height_) * 4u, 0);
}

void HeadlessSurface::clear(const uint32_t argb)
{
    const auto a = channel(argb, 24);
    const auto r = channel(argb, 16);
    const auto g = channel(argb, 8);
    const auto b = channel(argb, 0);
    for (auto i = size_t{0}; i < pixels_.size(); i += 4)
    {
        pixels_[i + 0] = r;
        pixels_[i + 1] = g;
        pixels_[i + 2] = b;
        pixels_[i + 3] = a;
    }
}

int HeadlessSurface::width() const
{
    return width_;
}

int HeadlessSurface::height() const
{
    return height_;
}

const std::vector<unsigned char>& HeadlessSurface::pixels() const
{
    return pixels_;
}

void HeadlessSurface::blend(const int x, const int y, const float coverage)
{
    if (x < 0 || y < 0 || x >= width_ || y >= height_ || coverage <= 0.f)
        return;

    const auto src_alpha = std::clamp(coverage, 0.f, 1.f) * (static_cast<float>(a_) / 255.f);
    const auto idx = (static_cast<size_t>(y) * static_cast<size_t>(width_) + static_cast<size_t>(x)) * 4u;
    const auto dst_alpha = static_cast<float>(pixels_[idx + 3]) / 255.f;
    const auto out_alpha = src_alpha + dst_alpha * (1.f - src_alpha);
    if (out_alpha <= 0.f)
        return;

    const auto mix = [src_alpha, dst_alpha, out_alpha](const unsigned char dst, const unsigned char src) {
        const auto src_channel = static_cast<float>(src) / 255.f;
        const auto dst_channel = static_cast<float>(dst) / 255.f;
        const auto out_channel = (src_channel * src_alpha + dst_channel * dst_alpha * (1.f - src_alpha)) / out_alpha;
        return static_cast<unsigned char>(std::lround(std::clamp(out_channel, 0.f, 1.f) * 255.f));
    };
    pixels_[idx + 0] = mix(pixels_[idx + 0], r_);
    pixels_[idx + 1] = mix(pixels_[idx + 1], g_);
    pixels_[idx + 2] = mix(pixels_[idx + 2], b_);
    pixels_[idx + 3] = static_cast<unsigned char>(std::lround(std::clamp(out_alpha, 0.f, 1.f) * 255.f));
}

void HeadlessSurface::fill_rect(const float x, const float y, const float w, const float h)
{
    const auto x0 = static_cast<int>(std::floor(x));
    const auto y0 = static_cast<int>(std::floor(y));
    const auto x1 = static_cast<int>(std::ceil(x + w));
    const auto y1 = static_cast<int>(std::ceil(y + h));
    for (auto py = y0; py < y1; ++py)
        for (auto px = x0; px < x1; ++px)
            blend(px, py, 1.f);
}

void HeadlessSurface::draw_rect(const float x, const float y, const float w, const float h, const float width)
{
    const auto t = std::max(1.f, width);
    fill_rect(x, y, w, t); // top
    fill_rect(x, y + h - t, w, t); // bottom
    fill_rect(x, y, t, h); // left
    fill_rect(x + w - t, y, t, h); // right
}

void HeadlessSurface::draw_line(const float x1, const float y1, const float x2, const float y2, const float width)
{
    // Thin axis-friendly line via dense sampling; adequate for rules/fraction bars.
    const auto dx = x2 - x1;
    const auto dy = y2 - y1;
    const auto steps = std::max(1, static_cast<int>(std::ceil(std::max(std::fabs(dx), std::fabs(dy)))));
    const auto half = std::max(1.f, width) / 2.f;
    for (auto i = 0; i <= steps; ++i)
    {
        const auto t = static_cast<float>(i) / static_cast<float>(steps);
        fill_rect(x1 + dx * t - half, y1 + dy * t - half, width, width);
    }
}

void HeadlessSurface::draw_round_rect(const float x, const float y, const float w, const float h, float, float, const float width)
{
    draw_rect(x, y, w, h, width);
}

void HeadlessSurface::fill_round_rect(const float x, const float y, const float w, const float h, float, float)
{
    fill_rect(x, y, w, h);
}

void HeadlessSurface::draw_glyph(const GlyphBitmap& glyph, const float pen_x, const float pen_y, float /*angle*/)
{
    if (!glyph.buffer || glyph.width <= 0 || glyph.height <= 0)
        return;

    const auto origin_x = static_cast<int>(std::lround(pen_x)) + glyph.left;
    const auto origin_y = static_cast<int>(std::lround(pen_y)) - glyph.top;
    for (auto row = 0; row < glyph.height; ++row)
    {
        const auto* const src = glyph.buffer + static_cast<size_t>(row) * static_cast<size_t>(glyph.pitch);
        for (auto col = 0; col < glyph.width; ++col)
            blend(origin_x + col, origin_y + row, static_cast<float>(src[col]) / 255.f);
    }
}

size_t HeadlessSurface::covered_pixels() const
{
    auto count = size_t{0};
    for (auto i = size_t{3}; i < pixels_.size(); i += 4)
        if (pixels_[i] != 0)
            ++count;

    return count;
}

bool HeadlessSurface::write_ppm(const std::string& path) const
{
    auto* const file = std::fopen(path.c_str(), "wb");
    if (!file)
        return false;

    std::fprintf(file, "P6\n%d %d\n255\n", width_, height_);
    for (auto i = size_t{0}; i + 3 < pixels_.size(); i += 4)
    {
        const unsigned char rgb[3] = {pixels_[i + 0], pixels_[i + 1], pixels_[i + 2]};
        std::fwrite(rgb, 1, 3, file);
    }

    return std::fclose(file) == 0;
}
