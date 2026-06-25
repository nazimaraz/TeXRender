#include "ft_font.hpp"
#include <string>
#include <utility>
#include "glyph_cache.hpp"

using namespace TeXRender;

FtFont::FtFont(std::string path)
    : path_{std::move(path)}
{}

bool FtFont::operator==(const Font& f) const
{
    const auto* const other = dynamic_cast<const FtFont*>(&f);
    return other && path_ == other->path_;
}

bool FtFont::operator!=(const Font& f) const
{
    return !(*this == f);
}

const std::string& FtFont::path() const
{
    return path_;
}
