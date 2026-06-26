#pragma once

#include <string>
#include "graphic/graphic.h"

namespace TeXRender
{
    class FtTextLayout final : public microtex::TextLayout
    {
    public:
        FtTextLayout(std::string text, std::string font_path, float font_size);

        void getBounds(microtex::Rect& bounds) override;
        void draw(microtex::Graphics2D& g2, float x, float y) override;

    private:
        std::string text_;
        std::string font_path_;
        float font_size_;
    };
} // namespace TeXRender
