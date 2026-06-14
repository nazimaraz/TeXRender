// Dependency-style example: a small reusable formula label that owns a parsed
// formula and draws it into an injected surface. This mirrors how an app might
// wrap texrender inside its own UI/widget layer.
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <texrender/formula_layout.hpp>
#include <texrender/headless_surface.hpp>
#include <texrender/latex_document.hpp>

#ifndef TEXRENDER_RES_DIR
#define TEXRENDER_RES_DIR "res"
#endif

namespace
{
    class FormulaLabel
    {
    public:
        FormulaLabel(const TeXRender::LatexDocument& document, std::string latex, const float text_size, const uint32_t color)
            : document_{document}
            , latex_{std::move(latex)}
            , text_size_{text_size}
            , color_{color}
        {
            refresh();
        }

        void set_formula(std::string latex)
        {
            latex_ = std::move(latex);
            refresh();
        }

        [[nodiscard]] TeXRender::FormulaBox box() const
        {
            return TeXRender::formula_box(*formula_);
        }

        void draw_baseline_aligned(const int x, const int baseline_y) const
        {
            TeXRender::draw_at_baseline(*formula_, x, baseline_y);
        }

    private:
        void refresh()
        {
            formula_ = document_.render(latex_, text_size_, color_);
        }

        const TeXRender::LatexDocument& document_;
        std::string latex_;
        float text_size_ = 48.f;
        uint32_t color_ = 0xff000000;
        std::optional<TeXRender::RenderHandle> formula_;
    };
} // namespace

int main()
{
    auto owned = std::make_unique<TeXRender::HeadlessSurface>();
    auto* const canvas = owned.get();
    const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};

    auto label = FormulaLabel{document, R"(\mathrm{Energy}=mc^2)", 42.f, 0xff202840};
    label.set_formula(R"(\Delta E = h\nu)");

    constexpr auto margin = 24;
    constexpr auto baseline = 80;
    const auto box = label.box();
    canvas->resize(box.width + 2 * margin, 120);
    canvas->clear(0xffffffff);
    label.draw_baseline_aligned(margin, baseline);

    if (!canvas->write_png("formula-label-widget.png"))
        return 1;

    std::printf("formula-label-widget.png: %dx%d baseline %.1f\n", box.width, box.height, box.baseline);
    return canvas->covered_pixels() > 0 ? 0 : 1;
}
