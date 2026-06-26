#include <algorithm>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <emscripten/emscripten.h>
#include <texrender/latex_document.hpp>
#include <texrender/svg_surface.hpp>

namespace
{
    constexpr auto resource_root = "/texrender-res";
    constexpr auto margin = 16;

    std::string g_svg;
    std::string g_error;

    [[nodiscard]] TeXRender::RenderOptions render_options(
        const float text_size, const std::uint32_t argb, const int wrap_width)
    {
        auto options = TeXRender::RenderOptions{};
        options.text_size = text_size;
        options.argb = argb;
        options.wrap_width = wrap_width;
        options.max_input_bytes = TeXRender::RenderOptions::default_untrusted_input_limit;
        options.max_text_size = TeXRender::RenderOptions::default_untrusted_max_text_size;
        options.max_wrap_width = TeXRender::RenderOptions::default_untrusted_max_wrap_width;
        return options;
    }

    [[nodiscard]] const char* fail(std::string message)
    {
        g_svg.clear();
        g_error = std::move(message);
        return nullptr;
    }
} // namespace

extern "C"
{
    EMSCRIPTEN_KEEPALIVE const char* texrender_render_svg(
        const char* latex, const float text_size, const std::uint32_t argb, const int wrap_width)
    {
        if (latex == nullptr)
            return fail("texrender: latex input is null");

        try
        {
            auto owned_surface = std::make_unique<TeXRender::SvgSurface>();
            auto& surface = *owned_surface;
            const auto document = TeXRender::LatexDocument{resource_root, std::move(owned_surface)};
            auto result = document.try_render_detailed(latex, render_options(text_size, argb, wrap_width));
            if (!result)
                return fail(result.error().message);

            const auto width = std::max(1, result->width() + margin * 2);
            const auto height = std::max(1, result->height() + margin * 2);
            surface.resize(width, height);
            surface.clear();
            result->draw(margin, margin);

            g_error.clear();
            g_svg = surface.svg();
            return g_svg.c_str();
        } catch (const std::exception& error)
        {
            return fail(error.what());
        } catch (...)
        {
            return fail("texrender: unknown render failure");
        }
    }

    EMSCRIPTEN_KEEPALIVE const char* texrender_last_error()
    {
        return g_error.c_str();
    }
}
