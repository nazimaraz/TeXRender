#include <array>
#include <cstdlib>
#include <optional>
#include <raylib.h>
#include "texgui/latex_document.hpp"

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
} // namespace

int main()
{
    InitWindow(1000, 560, "latex-renderer — live editor");
    SetTargetFPS(60);
    const auto document = TexGui::LatexDocument{TEXGUI_RES_DIR};
    auto source = std::string{k_presets[0]};
    auto caret = source.size();
    auto preset = std::size_t{0};
    auto text_size = 64.f;
    auto caret_scroll = 0;
    auto formula = std::optional<TexGui::RenderHandle>{};
    auto valid = true;
    auto rebuild = [&] {
        if (source.empty())
        {
            formula.reset();
            valid = true;
            return;
        }

        if (auto rendered = document.try_render(source, text_size, k_foreground))
        {
            formula = std::move(*rendered);
            valid = true;
        }
        else
        {
            valid = false;
        }
    };
    rebuild();
    const auto draw_scene = [&] {
        ClearBackground(k_background);
        constexpr auto margin = 24;
        constexpr auto panel_h = 64;
        const auto panel_w = GetScreenWidth() - 2 * margin;
        DrawRectangle(margin, margin, panel_w, panel_h, k_panel);
        DrawRectangleLines(margin, margin, panel_w, panel_h, valid ? k_ok : k_error);
        constexpr auto font_size = 24;
        constexpr auto text_x = margin + 16;
        constexpr auto text_y = margin + (panel_h - font_size) / 2;
        const auto inner_w = panel_w - 32;
        const auto caret_px = MeasureText(source.substr(0, caret).c_str(), font_size);
        if (const auto caret_limit = inner_w - 4; caret_px - caret_scroll > caret_limit)
            caret_scroll = caret_px - caret_limit;

        if (caret_px - caret_scroll < 0)
            caret_scroll = caret_px;

        caret_scroll = std::max(caret_scroll, 0);
        BeginScissorMode(text_x, margin, inner_w, panel_h);
        DrawText(source.c_str(), text_x - caret_scroll, text_y, font_size, k_ink);
        if (static_cast<int>(GetTime() * 2.) % 2 == 0)
            DrawRectangle(text_x - caret_scroll + caret_px, text_y, 2, font_size, k_ink);

        EndScissorMode();
        using namespace std::string_view_literals;
        auto status = "empty"sv;
        if (!source.empty())
            status = valid ? "rendered"sv : "parse error — showing last valid formula"sv;

        DrawText(status.data(), margin, margin + panel_h + 8, 18, valid ? k_ok : k_error);
        if (formula)
        {
            constexpr auto top = margin + panel_h + 48;
            const auto x = std::max(margin, (GetScreenWidth() - formula->width()) / 2);
            const auto y = std::max(top, top + (GetScreenHeight() - top - formula->height()) / 2);
            formula->draw(x, y);
        }
        else if (source.empty())
        {
            DrawText("type a LaTeX formula…", margin, GetScreenHeight() / 2, 22, k_muted);
        }

        DrawText("type    <- ->: caret    BACKSPACE: delete    TAB: preset    UP/DOWN: size    ESC: quit", margin,
            GetScreenHeight() - 30, 18, k_muted);
    };

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
        CloseWindow();
        return 0;
    }

    while (!WindowShouldClose())
    {
        auto dirty = false;
        for (auto codepoint = GetCharPressed(); codepoint != 0; codepoint = GetCharPressed())
        {
            if (codepoint >= 32)
            {
                insert_codepoint(source, caret, codepoint);
                dirty = true;
            }
        }

        if (key_repeats(KEY_BACKSPACE) && caret > 0)
        {
            erase_before(source, caret);
            dirty = true;
        }

        if (key_repeats(KEY_LEFT))
            caret = prev_boundary(source, caret);

        if (key_repeats(KEY_RIGHT))
            caret = next_boundary(source, caret);

        if (IsKeyPressed(KEY_HOME))
            caret = 0;

        if (IsKeyPressed(KEY_END))
            caret = source.size();

        if (IsKeyPressed(KEY_TAB))
        {
            preset = (preset + 1) % k_presets.size();
            source = k_presets[preset];
            caret = source.size();
            dirty = true;
        }

        if (IsKeyPressed(KEY_UP))
        {
            text_size = text_size + 4.f > 200.f ? 200.f : text_size + 4.f;
            dirty = true;
        }

        if (IsKeyPressed(KEY_DOWN))
        {
            text_size = text_size - 4.f < 12.f ? 12.f : text_size - 4.f;
            dirty = true;
        }

        if (dirty)
            rebuild();

        BeginDrawing();
        draw_scene();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
