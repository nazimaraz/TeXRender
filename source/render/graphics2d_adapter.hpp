#pragma once

#include "graphic/graphic.h"
#include "render/glyph_cache.hpp"
#include "render/transform2d.hpp"
#include "texrender/draw_surface.hpp"

namespace TeXRender
{
    class Graphics2DAdapter final : public tex::Graphics2D
    {
    public:
        explicit Graphics2DAdapter(IDrawSurface& surface);

        void setColor(tex::color c) override;
        [[nodiscard]] tex::color getColor() const override;
        void setStroke(const tex::Stroke& s) override;
        [[nodiscard]] const tex::Stroke& getStroke() const override;
        void setStrokeWidth(float w) override;
        [[nodiscard]] const tex::Font* getFont() const override;
        void setFont(const tex::Font* font) override;

        void translate(float dx, float dy) override;
        void scale(float sx, float sy) override;
        void rotate(float angle) override;
        void rotate(float angle, float px, float py) override;
        void reset() override;
        [[nodiscard]] float sx() const override;
        [[nodiscard]] float sy() const override;

        void drawChar(wchar_t c, float x, float y) override;
        void drawText(const std::wstring& t, float x, float y) override;
        void drawLine(float x1, float y1, float x2, float y2) override;
        void drawRect(float x, float y, float w, float h) override;
        void fillRect(float x, float y, float w, float h) override;
        void drawRoundRect(float x, float y, float w, float h, float rx, float ry) override;
        void fillRoundRect(float x, float y, float w, float h, float rx, float ry) override;

    private:
        [[nodiscard]] const GlyphCache::Glyph* raster(char32_t codepoint) const;

        IDrawSurface& surface_;
        GlyphCache& cache_;
        Transform2D transform_;
        tex::color color_ = tex::black;
        tex::Stroke stroke_;
        const tex::Font* font_{};
    };
} // namespace TeXRender
