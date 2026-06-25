#pragma once

#include "graphic/graphic.h"
#include "render/glyph_cache.hpp"
#include "render/transform2d.hpp"
#include "texrender/draw_surface.hpp"

namespace TeXRender
{
    class Graphics2DAdapter final : public microtex::Graphics2D
    {
    public:
        explicit Graphics2DAdapter(IDrawSurface& surface);

        void setColor(microtex::color c) override;
        [[nodiscard]] microtex::color getColor() const override;
        void setStroke(const microtex::Stroke& s) override;
        [[nodiscard]] const microtex::Stroke& getStroke() const override;
        void setStrokeWidth(float w) override;
        void setDash(const std::vector<float>& dash) override;
        [[nodiscard]] std::vector<float> getDash() override;
        [[nodiscard]] microtex::sptr<microtex::Font> getFont() const override;
        void setFont(const microtex::sptr<microtex::Font>& font) override;
        [[nodiscard]] float getFontSize() const override;
        void setFontSize(float size) override;

        void translate(float dx, float dy) override;
        void scale(float scale_x, float scale_y) override;
        void rotate(float angle) override;
        void rotate(float angle, float px, float py) override;
        void reset() override;
        [[nodiscard]] float sx() const override;
        [[nodiscard]] float sy() const override;

        void drawGlyph(microtex::u16 glyph, float x, float y) override;
        bool beginPath(microtex::i32 id) override;
        void moveTo(float x, float y) override;
        void lineTo(float x, float y) override;
        void cubicTo(float x1, float y1, float x2, float y2, float x3, float y3) override;
        void quadTo(float x1, float y1, float x2, float y2) override;
        void closePath() override;
        void fillPath(microtex::i32 id) override;
        void drawLine(float x1, float y1, float x2, float y2) override;
        void drawRect(float x, float y, float w, float h) override;
        void fillRect(float x, float y, float w, float h) override;
        void drawRoundRect(float x, float y, float w, float h, float rx, float ry) override;
        void fillRoundRect(float x, float y, float w, float h, float rx, float ry) override;

    private:
        [[nodiscard]] const GlyphCache::Glyph* raster(microtex::u16 glyph_index) const;

        IDrawSurface& surface_;
        GlyphCache& cache_;
        Transform2D transform_;
        microtex::color color_ = microtex::black;
        microtex::Stroke stroke_;
        std::vector<float> dash_;
        microtex::sptr<microtex::Font> font_{};
        float font_size_ = 12.f;
    };
} // namespace TeXRender
