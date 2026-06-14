#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <texrender/headless_surface.hpp>
#include <texrender/latex_document.hpp>

#ifndef TEXRENDER_RES_DIR
#define TEXRENDER_RES_DIR "res"
#endif

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    static auto owned = std::make_unique<TeXRender::HeadlessSurface>();
    static auto* const canvas = owned.get();
    static const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};

    if (size > TeXRender::RenderOptions::default_untrusted_input_limit)
        size = TeXRender::RenderOptions::default_untrusted_input_limit;

    auto input = std::string{reinterpret_cast<const char*>(data), size};
    auto options = TeXRender::RenderOptions{};
    options.text_size = 32.f;
    options.argb = 0xff000000;
    options.max_input_bytes = 4096;
    options.max_text_size = 96.f;
    options.max_wrap_width = 2048;

    if (auto formula = document.try_render(input, options))
    {
        const auto width = std::clamp(formula->width() + 16, 1, 2048);
        const auto height = std::clamp(formula->height() + 16, 1, 2048);
        canvas->resize(width, height);
        canvas->clear(0x00000000);
        formula->draw(8, 8);
    }

    return 0;
}
