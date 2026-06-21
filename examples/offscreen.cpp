// Render a formula offscreen — no window, GPU, or display — by injecting the
// dependency-free HeadlessSurface, then write the canvas to a PNG/PPM file. Works
// under any TEXRENDER_BACKEND, including dependency-free headless and SVG builds.
//
//   texrender_offscreen [latex] [out.png]
#include <cstdio>
#include <memory>
#include <string>
#include <texrender/headless_surface.hpp>
#include <texrender/latex_document.hpp>

#ifndef TEXRENDER_RES_DIR
#define TEXRENDER_RES_DIR "res"
#endif

namespace
{
    bool ends_with(const std::string& value, const std::string& suffix)
    {
        return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
} // namespace

int main(int argc, char** argv)
{
    const auto latex = std::string{argc > 1 ? argv[1] : R"(\int_{0}^{\infty} e^{-x^2}\,dx = \frac{\sqrt{\pi}}{2})"};
    const auto out = std::string{argc > 2 ? argv[2] : "formula.png"};

    //! [headless-render]
    auto owned = std::make_unique<TeXRender::HeadlessSurface>();
    auto& canvas = *owned;
    const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};

    const auto formula = document.try_render(latex, 64.f, 0xff1a1a2e);
    if (!formula)
    {
        std::fprintf(stderr, "could not parse: %s\n", latex.c_str());
        return 1;
    }

    constexpr auto margin = 16;
    canvas.resize(formula->width() + 2 * margin, formula->height() + 2 * margin);
    canvas.clear(0xffffffff); // opaque white
    formula->draw(margin, margin);
    //! [headless-render]

    const auto wrote = ends_with(out, ".ppm") ? canvas.write_ppm(out) : canvas.write_png(out);
    if (!wrote)
    {
        std::fprintf(stderr, "could not write: %s\n", out.c_str());
        return 1;
    }

    std::printf("wrote %s (%dx%d)\n", out.c_str(), canvas.width(), canvas.height());
    return 0;
}
