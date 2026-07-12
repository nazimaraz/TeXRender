#include <algorithm>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <raylib.h>
#include <emscripten/emscripten.h>
#include <texrender/latex_document.hpp>
#ifndef TEXRENDER_RES_DIR
#   define TEXRENDER_RES_DIR "/texrender-res"
#endif

namespace
{
    constexpr auto canvas_width = 960;
    constexpr auto canvas_height = 540;
    constexpr auto canvas_background = Color{255, 255, 255, 255};
    constexpr auto formula_argb = std::uint32_t{0xff1a1a2e};
    std::unique_ptr<TeXRender::LatexDocument> g_document;
    std::optional<TeXRender::RenderHandle> g_formula;
    std::string g_error;

    [[nodiscard]] TeXRender::RenderOptions render_options(const float text_size, const std::uint32_t argb, const int wrap_width)
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

    void draw_frame()
    {
        BeginDrawing();
        ClearBackground(canvas_background);
        if (g_formula)
        {
            const auto x = std::max(16, (GetScreenWidth() - g_formula->width()) / 2);
            const auto y = std::max(16, (GetScreenHeight() - g_formula->height()) / 2);
            g_formula->draw(x, y);
        }

        EndDrawing();
    }
} // namespace

extern "C" {
EMSCRIPTEN_KEEPALIVE int texrender_raylib_render(const char* latex, const float text_size, const std::uint32_t argb,
    const int wrap_width)
{
    if (!latex)
    {
        g_error = "texrender: latex input is null";
        g_formula.reset();
        return 0;
    }

    try
    {
        if (!g_document)
            g_document = std::make_unique<TeXRender::LatexDocument>(TEXRENDER_RES_DIR);

        auto result = g_document->try_render_detailed(latex, render_options(text_size, argb, wrap_width));
        if (!result)
        {
            g_error = result.error().message;
            g_formula.reset();
            return 0;
        }

        g_error.clear();
        g_formula = std::move(result).value();
        return 1;
    } catch (const std::exception& error)
    {
        g_error = error.what();
        g_formula.reset();
        return 0;
    } catch (...)
    {
        g_error = "texrender: unknown render failure";
        g_formula.reset();
        return 0;
    }
}

EMSCRIPTEN_KEEPALIVE const char* texrender_raylib_last_error()
{
    return g_error.c_str();
}

EMSCRIPTEN_KEEPALIVE int texrender_raylib_width()
{
    return g_formula ? g_formula->width() : 0;
}

EMSCRIPTEN_KEEPALIVE int texrender_raylib_height()
{
    return g_formula ? g_formula->height() : 0;
}
}

int main()
{
    InitWindow(canvas_width, canvas_height, "TeXRender raylib WebAssembly");
    SetTargetFPS(60);
    texrender_raylib_render(R"(\int_0^1 x^2\,dx = \frac{1}{3})", 72.f, formula_argb, 360);
    emscripten_set_main_loop(draw_frame, 0, true);
    return 0;
}
