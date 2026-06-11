#include <array>
#include <cstdlib>
#include <optional>
#include <raylib.h>
#include "texgui/latex_document.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#ifndef TEXGUI_RES_DIR
#define TEXGUI_RES_DIR "res"
#endif

namespace
{
    constexpr auto k_background = Color{250, 250, 250, 255};
    constexpr auto k_panel = Color{238, 238, 244, 255};
    constexpr auto k_ink = Color{26, 26, 46, 255};
    constexpr auto k_muted = Color{120, 120, 130, 255};
    constexpr auto k_ok = Color{40, 150, 90, 255};
    constexpr auto k_error = Color{200, 60, 60, 255};
    constexpr auto k_foreground = std::uint32_t{0xff1a1a2e};
    constexpr auto k_presets = std::array{
        R"(e^{i\pi} + 1 = 0)",
        R"(\sqrt{\frac{a^2+b^2}{c}})",
        R"(\int_{0}^{\infty} e^{-x^2}\,dx = \frac{\sqrt{\pi}}{2})",
        R"(\sum_{i=1}^{n} i = \frac{n(n+1)}{2})",
        R"(\frac{\partial}{\partial t}\,\Psi = \hat{H}\,\Psi)",
    };

    [[nodiscard]] std::size_t prev_boundary(const std::string& source, std::size_t pos)
    {
        if (pos == 0)
            return 0;

        --pos;
        while (pos > 0 && (static_cast<unsigned char>(source[pos]) & 0xc0) == 0x80)
            --pos;

        return pos;
    }

    [[nodiscard]] std::size_t next_boundary(const std::string& source, std::size_t pos)
    {
        if (pos >= source.size())
            return source.size();

        ++pos;
        while (pos < source.size() && (static_cast<unsigned char>(source[pos]) & 0xc0) == 0x80)
            ++pos;

        return pos;
    }

    void insert_codepoint(std::string& source, std::size_t& caret, const int codepoint)
    {
        auto size = 0;
        const auto* const utf8 = CodepointToUTF8(codepoint, &size);
        source.insert(caret, utf8, static_cast<std::size_t>(size));
        caret += static_cast<std::size_t>(size);
    }

    void erase_before(std::string& source, std::size_t& caret)
    {
        if (caret == 0)
            return;

        const auto start = prev_boundary(source, caret);
        source.erase(start, caret - start);
        caret = start;
    }

    [[nodiscard]] bool key_repeats(const int key)
    {
        return IsKeyPressed(key) || IsKeyPressedRepeat(key);
    }

    // All mutable state lives here (not in main's locals) so the per-frame
    // function can run under both drivers: a native while-loop, and the
    // browser's requestAnimationFrame via emscripten_set_main_loop — which
    // returns from main immediately, killing any locals.
    struct EditorState
    {
        TexGui::LatexDocument document{TEXGUI_RES_DIR};
        std::string source{k_presets[0]};
        std::size_t caret{source.size()};
        std::size_t preset{0};
        float text_size{64.f};
        int caret_scroll{0};
        std::optional<TexGui::RenderHandle> formula{};
        bool valid{true};
    };
    std::optional<EditorState> g_editor;

    void rebuild()
    {
        auto& ed = *g_editor;
        if (ed.source.empty())
        {
            ed.formula.reset();
            ed.valid = true;
            return;
        }

        if (auto rendered = ed.document.try_render(ed.source, ed.text_size, k_foreground))
        {
            ed.formula = std::move(*rendered);
            ed.valid = true;
        }
        else
        {
            ed.valid = false;
        }
    }

    void draw_scene()
    {
        auto& ed = *g_editor;
        ClearBackground(k_background);
        constexpr auto margin = 24;
        constexpr auto panel_h = 64;
        const auto panel_w = GetScreenWidth() - 2 * margin;
        DrawRectangle(margin, margin, panel_w, panel_h, k_panel);
        DrawRectangleLines(margin, margin, panel_w, panel_h, ed.valid ? k_ok : k_error);
        constexpr auto font_size = 24;
        constexpr auto text_x = margin + 16;
        constexpr auto text_y = margin + (panel_h - font_size) / 2;
        const auto inner_w = panel_w - 32;
        const auto caret_px = MeasureText(ed.source.substr(0, ed.caret).c_str(), font_size);
        if (const auto caret_limit = inner_w - 4; caret_px - ed.caret_scroll > caret_limit)
            ed.caret_scroll = caret_px - caret_limit;

        if (caret_px - ed.caret_scroll < 0)
            ed.caret_scroll = caret_px;

        ed.caret_scroll = std::max(ed.caret_scroll, 0);
        BeginScissorMode(text_x, margin, inner_w, panel_h);
        DrawText(ed.source.c_str(), text_x - ed.caret_scroll, text_y, font_size, k_ink);
        if (static_cast<int>(GetTime() * 2.) % 2 == 0)
            DrawRectangle(text_x - ed.caret_scroll + caret_px, text_y, 2, font_size, k_ink);

        EndScissorMode();
        using namespace std::string_view_literals;
        auto status = "empty"sv;
        if (!ed.source.empty())
            status = ed.valid ? "rendered"sv : "parse error — showing last valid formula"sv;

        DrawText(status.data(), margin, margin + panel_h + 8, 18, ed.valid ? k_ok : k_error);
        if (ed.formula)
        {
            constexpr auto top = margin + panel_h + 48;
            const auto x = std::max(margin, (GetScreenWidth() - ed.formula->width()) / 2);
            const auto y = std::max(top, top + (GetScreenHeight() - top - ed.formula->height()) / 2);
            ed.formula->draw(x, y);
        }
        else if (ed.source.empty())
        {
            DrawText("type a LaTeX formula…", margin, GetScreenHeight() / 2, 22, k_muted);
        }

#ifdef __EMSCRIPTEN__
        constexpr auto hint = "type    <- ->: caret    BACKSPACE: delete    TAB: preset    UP/DOWN: size";
#else
        constexpr auto hint = "type    <- ->: caret    BACKSPACE: delete    TAB: preset    UP/DOWN: size    ESC: quit";
#endif
        DrawText(hint, margin, GetScreenHeight() - 30, 18, k_muted);
    }

    void update_draw_frame()
    {
        auto& ed = *g_editor;
        auto dirty = false;
        for (auto codepoint = GetCharPressed(); codepoint != 0; codepoint = GetCharPressed())
        {
            if (codepoint >= 32)
            {
                insert_codepoint(ed.source, ed.caret, codepoint);
                dirty = true;
            }
        }

        if (key_repeats(KEY_BACKSPACE) && ed.caret > 0)
        {
            erase_before(ed.source, ed.caret);
            dirty = true;
        }

        if (key_repeats(KEY_LEFT))
            ed.caret = prev_boundary(ed.source, ed.caret);

        if (key_repeats(KEY_RIGHT))
            ed.caret = next_boundary(ed.source, ed.caret);

        if (IsKeyPressed(KEY_HOME))
            ed.caret = 0;

        if (IsKeyPressed(KEY_END))
            ed.caret = ed.source.size();

        if (IsKeyPressed(KEY_TAB))
        {
            ed.preset = (ed.preset + 1) % k_presets.size();
            ed.source = k_presets[ed.preset];
            ed.caret = ed.source.size();
            dirty = true;
        }

        if (IsKeyPressed(KEY_UP))
        {
            ed.text_size = ed.text_size + 4.f > 200.f ? 200.f : ed.text_size + 4.f;
            dirty = true;
        }

        if (IsKeyPressed(KEY_DOWN))
        {
            ed.text_size = ed.text_size - 4.f < 12.f ? 12.f : ed.text_size - 4.f;
            dirty = true;
        }

        if (dirty)
            rebuild();

        BeginDrawing();
        draw_scene();
        EndDrawing();
    }
} // namespace

int main()
{
    InitWindow(1000, 560, "latex-renderer — live editor");
    g_editor.emplace();
    rebuild();

    if (const auto* const shot = std::getenv("TEXGUI_SCREENSHOT"))
    {
        const auto target = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
        BeginTextureMode(target);
        draw_scene();
        EndTextureMode();
        auto image = LoadImageFromTexture(target.texture);
        ImageFlipVertical(&image);
        ExportImage(image, shot);
        UnloadImage(image);
        UnloadRenderTexture(target);
        g_editor.reset();
        CloseWindow();
        return 0;
    }

#ifdef __EMSCRIPTEN__
    // The browser paces frames via requestAnimationFrame; this never returns
    // (the page owns the app's lifetime, so there is no teardown path).
    emscripten_set_main_loop(update_draw_frame, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose())
        update_draw_frame();

    // Free the GPU glyph textures while the window is still alive.
    g_editor.reset();
    CloseWindow();
#endif
    return 0;
}
