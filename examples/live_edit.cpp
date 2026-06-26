#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
#include <raylib.h>
#include <texrender/latex_document.hpp>

#ifndef TEXRENDER_RES_DIR
#   define TEXRENDER_RES_DIR "res"
#endif

namespace
{
    constexpr auto background = Color{247, 247, 244, 255};
    constexpr auto panel = Color{255, 255, 255, 255};
    constexpr auto canvas = Color{255, 255, 255, 255};
    constexpr auto border = Color{222, 222, 216, 255};
    constexpr auto border_active = Color{35, 87, 198, 255};
    constexpr auto text = Color{32, 33, 36, 255};
    constexpr auto muted = Color{96, 99, 104, 255};
    constexpr auto error = Color{179, 38, 30, 255};
    constexpr auto button = Color{35, 87, 198, 255};
    constexpr auto button_hover = Color{24, 74, 180, 255};

    enum class Focus
    {
        latex,
        size,
        wrap
    };

    struct ColorChoice
    {
        const char* name;
        std::uint32_t argb;
        Color raylib;
    };

    constexpr auto colors = std::array{
        ColorChoice{"Ink", 0xff1a1a2e, Color{26, 26, 46, 255}},
        ColorChoice{"Blue", 0xff2357c6, Color{35, 87, 198, 255}},
        ColorChoice{"Green", 0xff266c5b, Color{38, 108, 91, 255}},
        ColorChoice{"Red", 0xffb3261e, Color{179, 38, 30, 255}},
    };

    struct DemoState
    {
        TeXRender::LatexDocument document{TEXRENDER_RES_DIR};
        std::string latex{R"(\int_0^1 x^2\,dx = \frac{1}{3})"};
        std::string size_text{"72"};
        std::string wrap_text{"360"};
        std::size_t latex_caret{latex.size()};
        std::size_t size_caret{size_text.size()};
        std::size_t wrap_caret{wrap_text.size()};
        int latex_scroll{0};
        int size_scroll{0};
        int wrap_scroll{0};
        std::size_t color_index = 0;
        Focus focus = Focus::latex;
        std::optional<TeXRender::RenderHandle> formula;
        std::string status{"Ready"};
        bool has_error = false;
    };

    [[nodiscard]] bool contains(const Rectangle rect, const Vector2 point)
    {
        return CheckCollisionPointRec(point, rect);
    }

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

    [[nodiscard]] bool key_repeats(const int key)
    {
        return IsKeyPressed(key) || IsKeyPressedRepeat(key);
    }

    [[nodiscard]] bool insert_codepoint(std::string& source, std::size_t& caret, const int codepoint, const std::size_t max_size)
    {
        auto size = 0;
        const auto* const utf8 = CodepointToUTF8(codepoint, &size);
        if (size <= 0 || source.size() + static_cast<std::size_t>(size) > max_size)
            return false;

        caret = std::min(caret, source.size());
        source.insert(caret, utf8, static_cast<std::size_t>(size));
        caret += static_cast<std::size_t>(size);
        return true;
    }

    [[nodiscard]] bool erase_before(std::string& source, std::size_t& caret)
    {
        if (caret == 0)
            return false;

        const auto start = prev_boundary(source, caret);
        source.erase(start, caret - start);
        caret = start;
        return true;
    }

    [[nodiscard]] bool erase_at(std::string& source, const std::size_t caret)
    {
        if (caret >= source.size())
            return false;

        const auto end = next_boundary(source, caret);
        source.erase(caret, end - caret);
        return true;
    }

    [[nodiscard]] int parse_int(const std::string& value, const int fallback)
    {
        if (value.empty())
            return fallback;

        auto parsed = fallback;
        if (std::sscanf(value.c_str(), "%d", &parsed) != 1)
            return fallback;

        return parsed;
    }

    [[nodiscard]] float text_size(const DemoState& state)
    {
        return static_cast<float>(std::clamp(parse_int(state.size_text, 72), 8, 256));
    }

    [[nodiscard]] int wrap_width(const DemoState& state)
    {
        return std::clamp(parse_int(state.wrap_text, 0), 0, 4096);
    }

    [[nodiscard]] TeXRender::RenderOptions render_options(const DemoState& state)
    {
        auto options = TeXRender::RenderOptions{};
        options.text_size = text_size(state);
        options.argb = colors[state.color_index].argb;
        options.wrap_width = wrap_width(state);
        options.max_input_bytes = TeXRender::RenderOptions::default_untrusted_input_limit;
        options.max_text_size = TeXRender::RenderOptions::default_untrusted_max_text_size;
        options.max_wrap_width = TeXRender::RenderOptions::default_untrusted_max_wrap_width;
        return options;
    }

    void render_formula(DemoState& state)
    {
        auto result = state.document.try_render_detailed(state.latex, render_options(state));
        if (!result)
        {
            state.status = result.error().message.empty() ? "Render failed" : result.error().message;
            state.has_error = true;
            return;
        }

        state.formula = std::move(result).value();
        state.status = "Ready: " + std::to_string(state.formula->width()) + " x " + std::to_string(state.formula->height());
        state.has_error = false;
    }

    [[nodiscard]] std::string& focused_value(DemoState& state)
    {
        if (state.focus == Focus::size)
            return state.size_text;

        if (state.focus == Focus::wrap)
            return state.wrap_text;

        return state.latex;
    }

    [[nodiscard]] std::size_t& focused_caret(DemoState& state)
    {
        if (state.focus == Focus::size)
            return state.size_caret;

        if (state.focus == Focus::wrap)
            return state.wrap_caret;

        return state.latex_caret;
    }

    [[nodiscard]] std::size_t focused_max_size(const DemoState& state)
    {
        if (state.focus == Focus::size)
            return 3;

        if (state.focus == Focus::wrap)
            return 4;

        return 512;
    }

    [[nodiscard]] bool focused_accepts_codepoint(const DemoState& state, const int codepoint)
    {
        if (state.focus == Focus::latex)
            return codepoint >= 32;

        return codepoint >= '0' && codepoint <= '9';
    }

    void handle_text_input(DemoState& state)
    {
        auto& active = focused_value(state);
        auto& caret = focused_caret(state);
        caret = std::min(caret, active.size());

        auto dirty = false;
        for (auto codepoint = GetCharPressed(); codepoint != 0; codepoint = GetCharPressed())
        {
            if (focused_accepts_codepoint(state, codepoint))
                dirty = insert_codepoint(active, caret, codepoint, focused_max_size(state)) || dirty;
        }

        if (key_repeats(KEY_BACKSPACE))
            dirty = erase_before(active, caret) || dirty;

        if (key_repeats(KEY_DELETE))
            dirty = erase_at(active, caret) || dirty;

        if (key_repeats(KEY_LEFT))
            caret = prev_boundary(active, caret);

        if (key_repeats(KEY_RIGHT))
            caret = next_boundary(active, caret);

        if (IsKeyPressed(KEY_HOME))
            caret = 0;

        if (IsKeyPressed(KEY_END))
            caret = active.size();

        if (dirty)
            render_formula(state);
    }

    [[nodiscard]] std::size_t caret_from_mouse(const std::string& value, const Rectangle rect, const int font_size,
        const int scroll, const float mouse_x)
    {
        const auto local_x = static_cast<int>(mouse_x - rect.x - 10.f) + scroll;
        if (local_x <= 0)
            return 0;

        auto best = value.size();
        auto best_distance = std::abs(MeasureText(value.c_str(), font_size) - local_x);
        for (auto pos = std::size_t{0}; pos < value.size(); pos = next_boundary(value, pos))
        {
            const auto width = MeasureText(value.substr(0, pos).c_str(), font_size);
            const auto distance = std::abs(width - local_x);
            if (distance < best_distance)
            {
                best = pos;
                best_distance = distance;
            }
        }

        return best;
    }

    void draw_label(const char* label, const int x, const int y)
    {
        DrawText(label, x, y, 14, muted);
    }

    void draw_field(const Rectangle rect, const std::string& value, const bool active, std::size_t caret, int& scroll,
        const int font_size = 18)
    {
        DrawRectangleRounded(rect, 0.08f, 8, panel);
        DrawRectangleRoundedLinesEx(rect, 0.08f, 8, 1.f, active ? border_active : border);
        caret = std::min(caret, value.size());
        const auto inner_x = static_cast<int>(rect.x + 10.f);
        const auto inner_y = static_cast<int>(rect.y);
        const auto inner_w = static_cast<int>(rect.width - 20.f);
        const auto caret_px = MeasureText(value.substr(0, caret).c_str(), font_size);
        if (caret_px - scroll > inner_w - 4)
            scroll = caret_px - (inner_w - 4);

        if (caret_px - scroll < 0)
            scroll = caret_px;

        scroll = std::max(scroll, 0);
        const auto text_y = static_cast<int>(rect.y + (rect.height - static_cast<float>(font_size)) / 2.f);
        BeginScissorMode(inner_x, inner_y, inner_w, static_cast<int>(rect.height));
        DrawText(value.c_str(), inner_x - scroll, text_y, font_size, text);
        if (active && static_cast<int>(GetTime() * 2.0) % 2 == 0)
            DrawRectangle(inner_x - scroll + caret_px, text_y, 2, font_size, text);

        EndScissorMode();
    }

    void draw_button(const Rectangle rect, const char* label)
    {
        const auto mouse = GetMousePosition();
        DrawRectangleRounded(rect, 0.12f, 10, contains(rect, mouse) ? button_hover : button);
        const auto font_size = 18;
        const auto text_width = MeasureText(label, font_size);
        DrawText(label, static_cast<int>(rect.x + (rect.width - text_width) / 2), static_cast<int>(rect.y + 12), font_size, WHITE);
    }

    void draw_ui(DemoState& state)
    {
        ClearBackground(background);
        constexpr auto left_width = 420.f;
        DrawRectangleRec(Rectangle{0.f, 0.f, left_width, static_cast<float>(GetScreenHeight())}, panel);
        DrawLine(static_cast<int>(left_width), 0, static_cast<int>(left_width), GetScreenHeight(), border);
        DrawText("TeXRender live_edit", 28, 28, 24, text);
        DrawText("Native executable", 28, 60, 18, muted);
        constexpr auto latex_rect = Rectangle{28.f, 112.f, 364.f, 150.f};
        constexpr auto size_rect = Rectangle{28.f, 306.f, 170.f, 40.f};
        constexpr auto wrap_rect = Rectangle{222.f, 306.f, 170.f, 40.f};
        constexpr auto render_rect = Rectangle{28.f, 438.f, 364.f, 42.f};
        draw_label("LaTeX", 28, 90);
        draw_field(latex_rect, state.latex, state.focus == Focus::latex, state.latex_caret, state.latex_scroll, 18);
        draw_label("Size", 28, 284);
        draw_field(size_rect, state.size_text, state.focus == Focus::size, state.size_caret, state.size_scroll);
        draw_label("Wrap width", 222, 284);
        draw_field(wrap_rect, state.wrap_text, state.focus == Focus::wrap, state.wrap_caret, state.wrap_scroll);
        draw_label("Color", 28, 366);
        for (auto i = std::size_t{0}; i < colors.size(); ++i)
        {
            const auto rect = Rectangle{28.f + static_cast<float>(i) * 48.f, 390.f, 34.f, 34.f};
            DrawRectangleRounded(rect, 0.18f, 8, colors[i].raylib);
            DrawRectangleRoundedLinesEx(rect, 0.18f, 8, i == state.color_index ? 3.f : 1.f, i == state.color_index ? text : border);
        }

        draw_button(render_rect, "Render");
        DrawText(state.status.c_str(), 28, 500, 16, state.has_error ? error : muted);
        const auto output =
            Rectangle{452.f, 32.f, static_cast<float>(GetScreenWidth()) - 484.f, static_cast<float>(GetScreenHeight()) - 64.f};
        DrawRectangleRounded(output, 0.025f, 12, canvas);
        DrawRectangleRoundedLinesEx(output, 0.025f, 12, 1.f, border);
        if (state.formula)
        {
            const auto x =
                static_cast<int>(output.x + std::max(24.f, (output.width - static_cast<float>(state.formula->width())) / 2.f));
            const auto y =
                static_cast<int>(output.y + std::max(24.f, (output.height - static_cast<float>(state.formula->height())) / 2.f));
            state.formula->draw(x, y);
        }
    }

    void handle_mouse(DemoState& state)
    {
        if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            return;

        const auto mouse = GetMousePosition();
        const auto latex_rect = Rectangle{28.f, 112.f, 364.f, 150.f};
        const auto size_rect = Rectangle{28.f, 306.f, 170.f, 40.f};
        const auto wrap_rect = Rectangle{222.f, 306.f, 170.f, 40.f};
        const auto render_rect = Rectangle{28.f, 438.f, 364.f, 42.f};
        if (contains(latex_rect, mouse))
        {
            state.focus = Focus::latex;
            state.latex_caret = caret_from_mouse(state.latex, latex_rect, 18, state.latex_scroll, mouse.x);
        }
        else if (contains(size_rect, mouse))
        {
            state.focus = Focus::size;
            state.size_caret = caret_from_mouse(state.size_text, size_rect, 18, state.size_scroll, mouse.x);
        }
        else if (contains(wrap_rect, mouse))
        {
            state.focus = Focus::wrap;
            state.wrap_caret = caret_from_mouse(state.wrap_text, wrap_rect, 18, state.wrap_scroll, mouse.x);
        }
        else if (contains(render_rect, mouse))
        {
            render_formula(state);
        }

        for (auto i = std::size_t{0}; i < colors.size(); ++i)
        {
            if (const auto rect = Rectangle{28.f + static_cast<float>(i) * 48.f, 390.f, 34.f, 34.f}; contains(rect, mouse))
            {
                state.color_index = i;
                render_formula(state);
                break;
            }
        }
    }
} // namespace

int main()
{
    InitWindow(1100, 620, "TeXRender live_edit");
    SetTargetFPS(60);
    auto state = DemoState{};
    render_formula(state);
    while (!WindowShouldClose())
    {
        handle_mouse(state);
        handle_text_input(state);
        BeginDrawing();
        draw_ui(state);
        EndDrawing();
    }

    state.formula.reset();
    CloseWindow();
    return 0;
}
