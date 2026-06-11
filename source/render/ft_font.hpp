#pragma once

#include <string>
#include "graphic/graphic.h"

namespace TexGui
{
    class FtFont final : public tex::Font
    {
    public:
        FtFont(std::string family, std::string path, float size, int style = tex::PLAIN);

        [[nodiscard]] float getSize() const override;
        [[nodiscard]] tex::sptr<Font> deriveFont(int style) const override;
        [[nodiscard]] bool operator==(const Font& f) const override;
        [[nodiscard]] bool operator!=(const Font& f) const override;
        [[nodiscard]] const std::string& path() const;
        [[nodiscard]] int style() const;

    private:
        std::string family_;
        std::string path_;
        float size_;
        int style_;
    };
} // namespace TexGui
