#include "texrender/svg_surface.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <numeric>
#include <optional>
#include <sstream>
#include <string_view>
#include "backend/glyph_bitmap_validation.hpp"

using namespace TeXRender;

namespace
{
    struct Color
    {
        unsigned char r = 0;
        unsigned char g = 0;
        unsigned char b = 0;
        unsigned char a = 255;
    };

    [[nodiscard]] unsigned char channel(const std::uint32_t argb, const unsigned shift)
    {
        return static_cast<unsigned char>((argb >> shift) & 0xffu);
    }

    [[nodiscard]] Color color_from_argb(const std::uint32_t argb)
    {
        return Color{channel(argb, 16), channel(argb, 8), channel(argb, 0), channel(argb, 24)};
    }

    [[nodiscard]] std::string number(const float value)
    {
        auto output = std::ostringstream{};
        output.imbue(std::locale::classic()); // SVG numbers must use '.' regardless of the host locale
        output << std::fixed << std::setprecision(3) << value;
        return output.str();
    }

    [[nodiscard]] std::string paint_attributes(const std::uint32_t argb, const char* const paint_kind)
    {
        const auto c = color_from_argb(argb);
        auto attrs = std::ostringstream{};
        attrs.imbue(std::locale::classic()); // keep hex color digits ungrouped under any host locale
        attrs << paint_kind << "=\"#" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(c.r) << std::setw(2)
              << static_cast<int>(c.g) << std::setw(2) << static_cast<int>(c.b) << std::dec << '"';
        if (c.a != 255)
            attrs << ' ' << paint_kind << "-opacity=\"" << number(static_cast<float>(c.a) / 255.f) << '"';

        return attrs.str();
    }

    void append_u16_le(std::vector<unsigned char>& output, const std::uint16_t value)
    {
        output.push_back(static_cast<unsigned char>(value & 0xffu));
        output.push_back(static_cast<unsigned char>((value >> 8u) & 0xffu));
    }

    void append_u32_be(std::vector<unsigned char>& output, const std::uint32_t value)
    {
        output.push_back(static_cast<unsigned char>((value >> 24u) & 0xffu));
        output.push_back(static_cast<unsigned char>((value >> 16u) & 0xffu));
        output.push_back(static_cast<unsigned char>((value >> 8u) & 0xffu));
        output.push_back(static_cast<unsigned char>(value & 0xffu));
    }

    [[nodiscard]] std::uint32_t crc32_update(std::uint32_t crc, const unsigned char byte)
    {
        crc ^= byte;
        for (auto bit = 0; bit < 8; ++bit)
        {
            const auto mask = 0u - (crc & 1u);
            crc = (crc >> 1u) ^ (0xedb88320u & mask);
        }

        return crc;
    }

    [[nodiscard]] std::uint32_t png_crc(const std::string_view type, const std::vector<unsigned char>& data)
    {
        auto crc = std::accumulate(type.begin(), type.end(), 0xffffffffu, [](const std::uint32_t current, const char byte) {
            return crc32_update(current, static_cast<unsigned char>(byte));
        });
        crc = std::accumulate(data.begin(), data.end(), crc, [](const std::uint32_t current, const unsigned char byte) {
            return crc32_update(current, byte);
        });
        return crc ^ 0xffffffffu;
    }

    [[nodiscard]] std::uint32_t adler32(const std::vector<unsigned char>& data)
    {
        auto a = 1u;
        auto b = 0u;
        for (const auto byte : data)
        {
            constexpr auto mod_adler = 65'521u;
            a = (a + byte) % mod_adler;
            b = (b + a) % mod_adler;
        }

        return (b << 16u) | a;
    }

    [[nodiscard]] bool append_png_chunk(std::vector<unsigned char>& output, const std::string_view type,
        const std::vector<unsigned char>& data)
    {
        if (type.size() != 4 || data.size() > std::numeric_limits<std::uint32_t>::max())
            return false;

        append_u32_be(output, static_cast<std::uint32_t>(data.size()));
        output.insert(output.end(), type.begin(), type.end());
        output.insert(output.end(), data.begin(), data.end());
        append_u32_be(output, png_crc(type, data));
        return true;
    }

    [[nodiscard]] bool byte_count_for_rgba(const int width, const int height, const std::size_t max_size, std::size_t& byte_count)
    {
        if (width <= 0 || height <= 0)
            return false;

        const auto max_pixels = max_size / 4u;
        const auto glyph_width = static_cast<std::size_t>(width);
        const auto glyph_height = static_cast<std::size_t>(height);
        if (glyph_height > max_pixels / glyph_width)
            return false;

        byte_count = glyph_width * glyph_height * 4u;
        return true;
    }

    [[nodiscard]] std::optional<std::vector<unsigned char>> zlib_store(const std::vector<unsigned char>& data)
    {
        const auto blocks = data.empty() ? std::size_t{0} : 1u + (data.size() - 1u) / 65'535u;
        constexpr auto zlib_overhead = std::size_t{8};
        constexpr auto max_size = std::numeric_limits<std::size_t>::max();
        if (data.size() > max_size - zlib_overhead || blocks > (max_size - data.size() - zlib_overhead) / 5u)
            return std::nullopt;

        const auto output_size = data.size() + blocks * 5u + zlib_overhead;
        if (output_size > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
            return std::nullopt;

        auto output = std::vector<unsigned char>{};
        output.reserve(output_size);
        output.push_back(0x78u);
        output.push_back(0x01u);
        auto offset = std::size_t{0};
        while (offset < data.size())
        {
            const auto remaining = data.size() - offset;
            const auto block_size = static_cast<std::uint16_t>(std::min<std::size_t>(remaining, 65'535u));
            const auto final_block = offset + block_size == data.size();
            output.push_back(final_block ? 0x01u : 0x00u);
            append_u16_le(output, block_size);
            append_u16_le(output, static_cast<std::uint16_t>(~block_size));
            output.insert(output.end(), data.begin() + static_cast<std::ptrdiff_t>(offset),
                data.begin() + static_cast<std::ptrdiff_t>(offset + block_size));
            offset += block_size;
        }

        append_u32_be(output, adler32(data));
        return output;
    }

    [[nodiscard]] std::optional<std::vector<unsigned char>> png_rgba(const int width, const int height,
        const std::vector<unsigned char>& rgba)
    {
        if (auto expected_bytes = std::size_t{0};
            !byte_count_for_rgba(width, height, rgba.max_size(), expected_bytes) || rgba.size() != expected_bytes)
            return std::nullopt;

        auto png = std::vector<unsigned char>{0x89u, 0x50u, 0x4eu, 0x47u, 0x0du, 0x0au, 0x1au, 0x0au};
        auto ihdr = std::vector<unsigned char>{};
        append_u32_be(ihdr, static_cast<std::uint32_t>(width));
        append_u32_be(ihdr, static_cast<std::uint32_t>(height));
        ihdr.push_back(8u);
        ihdr.push_back(6u);
        ihdr.push_back(0u);
        ihdr.push_back(0u);
        ihdr.push_back(0u);
        if (!append_png_chunk(png, "IHDR", ihdr))
            return std::nullopt;

        auto scanlines = std::vector<unsigned char>{};
        const auto stride = static_cast<std::size_t>(width) * 4u;
        if (stride == std::numeric_limits<std::size_t>::max())
            return std::nullopt;

        const auto row_size = stride + 1u;
        if (static_cast<std::size_t>(height) > scanlines.max_size() / row_size)
            return std::nullopt;

        scanlines.reserve(row_size * static_cast<std::size_t>(height));
        for (auto y = 0; y < height; ++y)
        {
            scanlines.push_back(0u);
            const auto begin = rgba.begin() + static_cast<std::ptrdiff_t>(static_cast<std::size_t>(y) * stride);
            scanlines.insert(scanlines.end(), begin, begin + static_cast<std::ptrdiff_t>(stride));
        }

        if (const auto idat = zlib_store(scanlines);
            !idat || !append_png_chunk(png, "IDAT", *idat) || !append_png_chunk(png, "IEND", {}))
            return std::nullopt;

        return png;
    }

    [[nodiscard]] std::string base64(const std::vector<unsigned char>& input)
    {
        static constexpr auto table = std::string_view{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};
        auto output = std::string{};
        output.reserve((input.size() + 2u) / 3u * 4u);
        auto i = std::size_t{0};
        while (i < input.size())
        {
            const auto a = input[i++];
            const auto b = i < input.size() ? input[i++] : 0u;
            const auto c = i < input.size() ? input[i++] : 0u;
            const auto triple =
                (static_cast<std::uint32_t>(a) << 16u) | (static_cast<std::uint32_t>(b) << 8u) | static_cast<std::uint32_t>(c);
            output.push_back(table[(triple >> 18u) & 0x3fu]);
            output.push_back(table[(triple >> 12u) & 0x3fu]);
            output.push_back(table[(triple >> 6u) & 0x3fu]);
            output.push_back(table[triple & 0x3fu]);
        }

        if (const auto remainder = input.size() % 3u; remainder != 0u)
        {
            output[output.size() - 1u] = '=';
            if (remainder == 1u)
                output[output.size() - 2u] = '=';
        }

        return output;
    }

    [[nodiscard]] constexpr float to_degrees(const float radians)
    {
        return radians * (180.f / 3.14159265358979323846f);
    }

    [[nodiscard]] bool finite_line(const float x1, const float y1, const float x2, const float y2, const float width)
    {
        return std::isfinite(x1) && std::isfinite(y1) && std::isfinite(x2) && std::isfinite(y2) && std::isfinite(width);
    }

    [[nodiscard]] bool finite_rect(const float x, const float y, const float w, const float h)
    {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(w) && std::isfinite(h) && w > 0.f && h > 0.f;
    }
} // namespace

SvgSurface::SvgSurface(const int width, const int height)
{
    resize(width, height);
}

void SvgSurface::set_color(const std::uint32_t argb)
{
    color_ = argb;
}

void SvgSurface::resize(const int canvas_width, const int canvas_height)
{
    width_ = std::max(0, canvas_width);
    height_ = std::max(0, canvas_height);
}

void SvgSurface::clear(const std::uint32_t argb)
{
    elements_.clear();
    background_.reset();
    if (channel(argb, 24) != 0)
        background_ = argb;
}

int SvgSurface::width() const noexcept
{
    return width_;
}

int SvgSurface::height() const noexcept
{
    return height_;
}

std::size_t SvgSurface::element_count() const noexcept
{
    return elements_.size() + (background_.has_value() ? 1u : 0u);
}

void SvgSurface::draw_line(const float x1, const float y1, const float x2, const float y2, const float stroke_width)
{
    if (!finite_line(x1, y1, x2, y2, stroke_width))
        return;

    elements_.push_back("<line x1=\"" + number(x1) + "\" y1=\"" + number(y1) + "\" x2=\"" + number(x2) + "\" y2=\"" + number(y2) +
                        "\" " + paint_attributes(color_, "stroke") + " stroke-width=\"" + number(stroke_width) +
                        "\" stroke-linecap=\"round\"/>");
}

void SvgSurface::draw_rect(const float x, const float y, const float w, const float h, const float stroke_width)
{
    if (!finite_rect(x, y, w, h) || !std::isfinite(stroke_width))
        return;

    elements_.push_back("<rect x=\"" + number(x) + "\" y=\"" + number(y) + "\" width=\"" + number(w) + "\" height=\"" + number(h) +
                        "\" fill=\"none\" " + paint_attributes(color_, "stroke") + " stroke-width=\"" + number(stroke_width) +
                        "\"/>");
}

void SvgSurface::fill_rect(const float x, const float y, const float w, const float h)
{
    if (!finite_rect(x, y, w, h))
        return;

    elements_.push_back("<rect x=\"" + number(x) + "\" y=\"" + number(y) + "\" width=\"" + number(w) + "\" height=\"" + number(h) +
                        "\" " + paint_attributes(color_, "fill") + "/>");
}

void SvgSurface::draw_round_rect(const float x, const float y, const float w, const float h, const float rx, const float ry,
    const float stroke_width)
{
    if (!finite_rect(x, y, w, h) || !std::isfinite(rx) || !std::isfinite(ry) || !std::isfinite(stroke_width))
        return;

    elements_.push_back("<rect x=\"" + number(x) + "\" y=\"" + number(y) + "\" width=\"" + number(w) + "\" height=\"" + number(h) +
                        "\" rx=\"" + number(rx) + "\" ry=\"" + number(ry) + "\" fill=\"none\" " +
                        paint_attributes(color_, "stroke") + " stroke-width=\"" + number(stroke_width) + "\"/>");
}

void SvgSurface::fill_round_rect(const float x, const float y, const float w, const float h, const float rx, const float ry)
{
    if (!finite_rect(x, y, w, h) || !std::isfinite(rx) || !std::isfinite(ry))
        return;

    elements_.push_back("<rect x=\"" + number(x) + "\" y=\"" + number(y) + "\" width=\"" + number(w) + "\" height=\"" + number(h) +
                        "\" rx=\"" + number(rx) + "\" ry=\"" + number(ry) + "\" " + paint_attributes(color_, "fill") + "/>");
}

void SvgSurface::draw_glyph(const GlyphBitmap& glyph, const float pen_x, const float pen_y, const float angle)
{
    if (!detail::valid_glyph_bitmap_for_rgba(glyph) || !std::isfinite(pen_x) || !std::isfinite(pen_y) || !std::isfinite(angle))
        return;

    const auto c = color_from_argb(color_);
    auto rgba_size = std::size_t{0};
    auto rgba = std::vector<unsigned char>{};
    if (!byte_count_for_rgba(glyph.width, glyph.height, rgba.max_size(), rgba_size))
        return;
    rgba.resize(rgba_size);
    for (auto row = 0; row < glyph.height; ++row)
    {
        for (auto col = 0; col < glyph.width; ++col)
        {
            const auto coverage =
                glyph.buffer[static_cast<std::size_t>(row) * static_cast<std::size_t>(glyph.pitch) + static_cast<std::size_t>(col)];
            const auto dst =
                (static_cast<std::size_t>(row) * static_cast<std::size_t>(glyph.width) + static_cast<std::size_t>(col)) * 4u;
            rgba[dst + 0] = c.r;
            rgba[dst + 1] = c.g;
            rgba[dst + 2] = c.b;
            rgba[dst + 3] = static_cast<unsigned char>((static_cast<unsigned>(coverage) * static_cast<unsigned>(c.a)) / 255u);
        }
    }

    const auto x = pen_x + static_cast<float>(glyph.left);
    const auto y = pen_y - static_cast<float>(glyph.top);
    auto transform = std::string{};
    if (std::fabs(angle) > 0.0001f)
        transform = " transform=\"rotate(" + number(to_degrees(angle)) + ' ' + number(pen_x) + ' ' + number(pen_y) + ")\"";

    const auto png = png_rgba(glyph.width, glyph.height, rgba);
    if (!png)
        return;

    elements_.push_back("<image x=\"" + number(x) + "\" y=\"" + number(y) + "\" width=\"" + std::to_string(glyph.width) +
                        "\" height=\"" + std::to_string(glyph.height) + "\" href=\"data:image/png;base64," + base64(*png) + "\"" +
                        transform + "/>");
}

std::string SvgSurface::svg() const
{
    auto out = std::ostringstream{};
    out.imbue(std::locale::classic()); // integer dimensions must not pick up locale digit grouping
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width_ << "\" height=\"" << height_ << "\" viewBox=\"0 0 "
        << width_ << ' ' << height_ << "\">";
    out << '\n';
    if (background_)
    {
        out << "  <rect width=\"100%\" height=\"100%\" " << paint_attributes(*background_, "fill") << "/>\n";
    }
    for (const auto& element : elements_)
        out << "  " << element << '\n';
    out << "</svg>\n";
    return out.str();
}

bool SvgSurface::write_svg(const std::string& path) const
{
    auto file = std::ofstream{path, std::ios::binary};
    if (!file)
        return false;

    file << svg();
    return static_cast<bool>(file);
}
