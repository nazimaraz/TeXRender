#include "render/ft_font.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include "render/glyph_cache.hpp"

using namespace TexGui;

namespace
{
    [[nodiscard]] std::string font_directory()
    {
        const auto base = detail::default_font_path();
        const auto slash = base.find_last_of('/');
        return slash == std::string::npos ? std::string{} : base.substr(0, slash);
    }

    [[nodiscard]] std::string_view variant_for(const int style, const std::array<std::string_view, 4>& variants)
    {
        const auto bold = (style & tex::BOLD) != 0;
        const auto italic = (style & tex::ITALIC) != 0;
        return variants[static_cast<size_t>((bold ? 1 : 0) | (italic ? 2 : 0))];
    }

    [[nodiscard]] bool contains(const std::string& text, const std::string_view needle)
    {
        return text.find(needle) != std::string::npos;
    }

    [[nodiscard]] std::string resolve_font(const std::string& family, const int style)
    {
        const auto dir = font_directory();
        if (dir.empty())
            return detail::default_font_path();

        auto lower = family;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](const unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        const auto file = [&] {
            using namespace std::string_view_literals;
            if (contains(lower, "mono") || contains(lower, "typewriter"))
                return "tt10.ttf"sv;

            if (contains(lower, "sans"))
                return variant_for(style, {"ss10.ttf"sv, "sb10.ttf"sv, "si10.ttf"sv, "sbi10.ttf"sv});

            if (contains(lower, "serif"))
                return variant_for(style, {"r10.ttf"sv, "bx10.ttf"sv, "i10.ttf"sv, "bi10.ttf"sv});

            return variant_for(style, {""sv, "bx10.ttf"sv, "i10.ttf"sv, "bi10.ttf"sv});
        }();
        if (file.empty())
            return detail::default_font_path();

        return dir + '/' + std::string{file};
    }
} // namespace

FtFont::FtFont(std::string family, std::string path, const float size, const int style)
    : family_{std::move(family)}
    , path_{std::move(path)}
    , size_{size}
    , style_{style}
{}

float FtFont::getSize() const
{
    return size_;
}

tex::sptr<tex::Font> FtFont::deriveFont(const int style) const
{
    if (family_.empty())
        return tex::sptrOf<FtFont>(family_, path_, size_, style);

    return tex::sptrOf<FtFont>(family_, resolve_font(family_, style), size_, style);
}

bool FtFont::operator==(const Font& f) const
{
    const auto* const other = dynamic_cast<const FtFont*>(&f);
    return other && size_ == other->size_ && style_ == other->style_ && path_ == other->path_;
}

bool FtFont::operator!=(const Font& f) const
{
    return !(*this == f);
}

const std::string& FtFont::path() const
{
    return path_;
}

int FtFont::style() const
{
    return style_;
}

tex::Font* tex::Font::create(const std::string& file, const float size)
{
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    return new FtFont{std::string{}, file, size};
}

tex::sptr<tex::Font> tex::Font::_create(const std::string& name, const int style, const float size)
{
    return tex::sptrOf<FtFont>(name, resolve_font(name, style), size, style);
}
