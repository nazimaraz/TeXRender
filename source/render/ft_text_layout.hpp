#pragma once

#include <string>
#include "graphic/graphic.h"
#include "render/ft_font.hpp"

namespace TeXRender
{
    class FtTextLayout final : public tex::TextLayout
    {
    public:
        FtTextLayout(std::wstring text, tex::sptr<FtFont> font);

        void getBounds(tex::Rect& bounds) override;
        void draw(tex::Graphics2D& g2, float x, float y) override;

    private:
        std::wstring text_;
        tex::sptr<FtFont> font_;
    };
} // namespace TeXRender
