#include "texrender/headless_surface.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>
#include <numeric>
#include <optional>
#include <string_view>
#include <vector>
#include "backend/glyph_bitmap_validation.hpp"

using namespace TeXRender;

namespace
{
    [[nodiscard]] bool finite_rect(const float x, const float y, const float w, const float h)
    {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(w) && std::isfinite(h) && w > 0.f && h > 0.f;
    }

    [[nodiscard]] int clamp_floor_to_canvas(const double value, const int max)
    {
        return static_cast<int>(std::clamp(std::floor(value), 0.0, static_cast<double>(max)));
    }

    [[nodiscard]] int clamp_ceil_to_canvas(const double value, const int max)
    {
        return static_cast<int>(std::clamp(std::ceil(value), 0.0, static_cast<double>(max)));
    }

    [[nodiscard]] bool pixel_byte_count(const int width, const int height, const size_t max_size, size_t& byte_count)
    {
        if (width <= 0 || height <= 0)
        {
            byte_count = 0;
            return true;
        }

        const auto max_pixels = max_size / 4u;
        const auto canvas_width = static_cast<size_t>(width);
        const auto canvas_height = static_cast<size_t>(height);
        if (canvas_height > max_pixels / canvas_width)
            return false;

        byte_count = canvas_width * canvas_height * 4u;
        return true;
    }

    [[nodiscard]] bool valid_canvas(const int width, const int height, const std::vector<unsigned char>& pixels)
    {
        auto expected_bytes = size_t{0};
        return pixel_byte_count(width, height, pixels.max_size(), expected_bytes) && expected_bytes > 0 &&
               pixels.size() == expected_bytes;
    }

    [[nodiscard]] bool clip_line_to_rect(double& x1, double& y1, double& x2, double& y2, const double min_x, const double min_y,
        const double max_x, const double max_y)
    {
        auto t0 = 0.0;
        auto t1 = 1.0;
        const auto dx = x2 - x1;
        const auto dy = y2 - y1;

        const auto clip = [&t0, &t1](const double p, const double q) {
            if (p == 0.0)
                return q >= 0.0;

            const auto r = q / p;
            if (p < 0.0)
            {
                if (r > t1)
                    return false;
                if (r > t0)
                    t0 = r;
            }
            else
            {
                if (r < t0)
                    return false;
                if (r < t1)
                    t1 = r;
            }
            return true;
        };

        if (!clip(-dx, x1 - min_x) || !clip(dx, max_x - x1) || !clip(-dy, y1 - min_y) || !clip(dy, max_y - y1))
            return false;

        const auto old_x1 = x1;
        const auto old_y1 = y1;
        x1 = old_x1 + t0 * dx;
        y1 = old_y1 + t0 * dy;
        x2 = old_x1 + t1 * dx;
        y2 = old_y1 + t1 * dy;
        return true;
    }

    [[nodiscard]] unsigned char channel(const uint32_t argb, const unsigned shift)
    {
        return static_cast<unsigned char>((argb >> shift) & 0xffu);
    }

    void append_u16_le(std::vector<unsigned char>& output, const uint16_t value)
    {
        output.push_back(static_cast<unsigned char>(value & 0xffu));
        output.push_back(static_cast<unsigned char>((value >> 8u) & 0xffu));
    }

    void append_u32_be(std::vector<unsigned char>& output, const uint32_t value)
    {
        output.push_back(static_cast<unsigned char>((value >> 24u) & 0xffu));
        output.push_back(static_cast<unsigned char>((value >> 16u) & 0xffu));
        output.push_back(static_cast<unsigned char>((value >> 8u) & 0xffu));
        output.push_back(static_cast<unsigned char>(value & 0xffu));
    }

    void write_u32_be(std::ofstream& output, const uint32_t value)
    {
        const unsigned char bytes[4] = {
            static_cast<unsigned char>((value >> 24u) & 0xffu),
            static_cast<unsigned char>((value >> 16u) & 0xffu),
            static_cast<unsigned char>((value >> 8u) & 0xffu),
            static_cast<unsigned char>(value & 0xffu),
        };
        output.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
    }

    [[nodiscard]] uint32_t crc32_update(uint32_t crc, const unsigned char byte)
    {
        crc ^= byte;
        for (auto bit = 0; bit < 8; ++bit)
        {
            const auto mask = 0u - (crc & 1u);
            crc = (crc >> 1u) ^ (0xedb88320u & mask);
        }
        return crc;
    }

    [[nodiscard]] uint32_t png_crc(const std::string_view type, const std::vector<unsigned char>& data)
    {
        auto crc = std::accumulate(type.begin(), type.end(), 0xffffffffu, [](const uint32_t current, const char byte) {
            return crc32_update(current, static_cast<unsigned char>(byte));
        });
        crc = std::accumulate(data.begin(), data.end(), crc, [](const uint32_t current, const unsigned char byte) {
            return crc32_update(current, byte);
        });
        return crc ^ 0xffffffffu;
    }

    [[nodiscard]] uint32_t adler32(const std::vector<unsigned char>& data)
    {
        constexpr auto mod_adler = 65521u;
        auto a = 1u;
        auto b = 0u;
        for (const auto byte : data)
        {
            a = (a + byte) % mod_adler;
            b = (b + a) % mod_adler;
        }
        return (b << 16u) | a;
    }

    [[nodiscard]] std::optional<std::vector<unsigned char>> zlib_store(const std::vector<unsigned char>& data)
    {
        const auto blocks = data.empty() ? size_t{0} : 1u + (data.size() - 1u) / 65535u;
        constexpr auto zlib_overhead = size_t{8};
        const auto max_size = std::numeric_limits<size_t>::max();
        if (data.size() > max_size - zlib_overhead || blocks > (max_size - data.size() - zlib_overhead) / 5u)
            return std::nullopt;

        const auto output_size = data.size() + blocks * 5u + zlib_overhead;
        if (output_size > static_cast<size_t>(std::numeric_limits<uint32_t>::max()))
            return std::nullopt;

        auto output = std::vector<unsigned char>{};
        output.reserve(output_size);
        output.push_back(0x78u);
        output.push_back(0x01u);

        auto offset = size_t{0};
        while (offset < data.size())
        {
            const auto remaining = data.size() - offset;
            const auto block_size = static_cast<uint16_t>(std::min<size_t>(remaining, 65535u));
            const auto final_block = offset + block_size == data.size();
            output.push_back(final_block ? 0x01u : 0x00u);
            append_u16_le(output, block_size);
            append_u16_le(output, static_cast<uint16_t>(~block_size));
            output.insert(output.end(), data.begin() + static_cast<std::ptrdiff_t>(offset),
                data.begin() + static_cast<std::ptrdiff_t>(offset + block_size));
            offset += block_size;
        }

        append_u32_be(output, adler32(data));
        return output;
    }

    bool write_png_chunk(std::ofstream& file, const std::string_view type, const std::vector<unsigned char>& data)
    {
        if (type.size() != 4 || data.size() > std::numeric_limits<uint32_t>::max())
            return false;

        write_u32_be(file, static_cast<uint32_t>(data.size()));
        file.write(type.data(), static_cast<std::streamsize>(type.size()));
        if (!data.empty())
            file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        write_u32_be(file, png_crc(type, data));
        return static_cast<bool>(file);
    }
} // namespace

void HeadlessSurface::set_color(const uint32_t argb)
{
    a_ = channel(argb, 24);
    r_ = channel(argb, 16);
    g_ = channel(argb, 8);
    b_ = channel(argb, 0);
}

void HeadlessSurface::resize(const int canvas_width, const int canvas_height)
{
    const auto new_width = std::max(0, canvas_width);
    const auto new_height = std::max(0, canvas_height);
    auto byte_count = size_t{0};
    if (!pixel_byte_count(new_width, new_height, pixels_.max_size(), byte_count))
    {
        width_ = 0;
        height_ = 0;
        pixels_.clear();
        return;
    }

    auto new_pixels = std::vector<unsigned char>(byte_count, 0);
    width_ = new_width;
    height_ = new_height;
    pixels_ = std::move(new_pixels);
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
    if (width_ <= 0 || height_ <= 0 || !finite_rect(x, y, w, h))
        return;

    const auto x_end = static_cast<double>(x) + static_cast<double>(w);
    const auto y_end = static_cast<double>(y) + static_cast<double>(h);
    if (!std::isfinite(x_end) || !std::isfinite(y_end))
        return;

    const auto x0 = clamp_floor_to_canvas(static_cast<double>(x), width_);
    const auto y0 = clamp_floor_to_canvas(static_cast<double>(y), height_);
    const auto x1 = clamp_ceil_to_canvas(x_end, width_);
    const auto y1 = clamp_ceil_to_canvas(y_end, height_);
    for (auto py = y0; py < y1; ++py)
        for (auto px = x0; px < x1; ++px)
            blend(px, py, 1.f);
}

void HeadlessSurface::draw_rect(const float x, const float y, const float w, const float h, const float stroke_width)
{
    const auto t = std::max(1.f, stroke_width);
    fill_rect(x, y, w, t); // top
    fill_rect(x, y + h - t, w, t); // bottom
    fill_rect(x, y, t, h); // left
    fill_rect(x + w - t, y, t, h); // right
}

void HeadlessSurface::draw_line(const float x1, const float y1, const float x2, const float y2, const float stroke_width)
{
    if (width_ <= 0 || height_ <= 0 || !std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(x2) || !std::isfinite(y2) ||
        !std::isfinite(stroke_width))
        return;

    // Thin axis-friendly line via dense sampling; adequate for rules/fraction bars.
    const auto max_thickness = static_cast<float>(static_cast<double>(std::max(width_, height_)) * 2.0 + 2.0);
    const auto thickness = std::clamp(stroke_width, 1.f, max_thickness);
    const auto half = static_cast<double>(thickness) / 2.0;
    auto clipped_x1 = static_cast<double>(x1);
    auto clipped_y1 = static_cast<double>(y1);
    auto clipped_x2 = static_cast<double>(x2);
    auto clipped_y2 = static_cast<double>(y2);
    if (!clip_line_to_rect(clipped_x1, clipped_y1, clipped_x2, clipped_y2, -half, -half, static_cast<double>(width_) + half,
            static_cast<double>(height_) + half))
        return;

    const auto dx = clipped_x2 - clipped_x1;
    const auto dy = clipped_y2 - clipped_y1;
    const auto steps = std::max(1, static_cast<int>(std::ceil(std::max(std::fabs(dx), std::fabs(dy)))));
    for (auto i = 0; i <= steps; ++i)
    {
        const auto t = static_cast<double>(i) / static_cast<double>(steps);
        fill_rect(static_cast<float>(clipped_x1 + dx * t - half), static_cast<float>(clipped_y1 + dy * t - half), thickness,
            thickness);
    }
}

void HeadlessSurface::draw_round_rect(const float x, const float y, const float w, const float h, float, float,
    const float stroke_width)
{
    draw_rect(x, y, w, h, stroke_width);
}

void HeadlessSurface::fill_round_rect(const float x, const float y, const float w, const float h, float, float)
{
    fill_rect(x, y, w, h);
}

void HeadlessSurface::draw_glyph(const GlyphBitmap& glyph, const float pen_x, const float pen_y, float /*angle*/)
{
    if (!detail::valid_glyph_bitmap(glyph) || !std::isfinite(pen_x) || !std::isfinite(pen_y))
        return;

    const auto origin_x = std::round(static_cast<double>(pen_x)) + static_cast<double>(glyph.left);
    const auto origin_y = std::round(static_cast<double>(pen_y)) - static_cast<double>(glyph.top);
    if (!std::isfinite(origin_x) || !std::isfinite(origin_y))
        return;

    for (auto row = 0; row < glyph.height; ++row)
    {
        const auto* const src = glyph.buffer + static_cast<size_t>(row) * static_cast<size_t>(glyph.pitch);
        for (auto col = 0; col < glyph.width; ++col)
        {
            const auto x = origin_x + static_cast<double>(col);
            const auto y = origin_y + static_cast<double>(row);
            if (x < 0.0 || y < 0.0 || x >= static_cast<double>(width_) || y >= static_cast<double>(height_))
                continue;
            blend(static_cast<int>(x), static_cast<int>(y), static_cast<float>(src[col]) / 255.f);
        }
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
    if (!valid_canvas(width_, height_, pixels_))
        return false;

    auto* const file = std::fopen(path.c_str(), "wb");
    if (!file)
        return false;

    auto ok = std::fprintf(file, "P6\n%d %d\n255\n", width_, height_) > 0;
    for (auto i = size_t{0}; ok && i + 3 < pixels_.size(); i += 4)
    {
        const unsigned char rgb[3] = {pixels_[i + 0], pixels_[i + 1], pixels_[i + 2]};
        ok = std::fwrite(rgb, 1, sizeof(rgb), file) == sizeof(rgb);
    }

    // Always close (no leaked FILE*), but report header/pixel write failures too.
    return std::fclose(file) == 0 && ok;
}

bool HeadlessSurface::write_png(const std::string& path) const
{
    if (!valid_canvas(width_, height_, pixels_))
        return false;

    auto file = std::ofstream{path, std::ios::binary};
    if (!file)
        return false;

    constexpr unsigned char signature[8] = {0x89u, 0x50u, 0x4eu, 0x47u, 0x0du, 0x0au, 0x1au, 0x0au};
    file.write(reinterpret_cast<const char*>(signature), sizeof(signature));

    auto ihdr = std::vector<unsigned char>{};
    ihdr.reserve(13);
    append_u32_be(ihdr, static_cast<uint32_t>(width_));
    append_u32_be(ihdr, static_cast<uint32_t>(height_));
    ihdr.push_back(8u); // bit depth
    ihdr.push_back(6u); // RGBA
    ihdr.push_back(0u); // zlib/deflate
    ihdr.push_back(0u); // no PNG filter
    ihdr.push_back(0u); // no interlace
    if (!write_png_chunk(file, "IHDR", ihdr))
        return false;

    auto scanlines = std::vector<unsigned char>{};
    const auto stride = static_cast<size_t>(width_) * 4u;
    if (stride == std::numeric_limits<size_t>::max())
        return false;

    const auto row_size = stride + 1u;
    if (static_cast<size_t>(height_) > scanlines.max_size() / row_size)
        return false;

    scanlines.reserve(row_size * static_cast<size_t>(height_));
    for (auto y = 0; y < height_; ++y)
    {
        scanlines.push_back(0u); // filter type 0
        const auto row_begin = pixels_.begin() + static_cast<std::ptrdiff_t>(static_cast<size_t>(y) * stride);
        scanlines.insert(scanlines.end(), row_begin, row_begin + static_cast<std::ptrdiff_t>(stride));
    }

    const auto idat = zlib_store(scanlines);
    if (!idat || !write_png_chunk(file, "IDAT", *idat))
        return false;

    return write_png_chunk(file, "IEND", {});
}
