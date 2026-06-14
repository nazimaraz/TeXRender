#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
#include <raylib.h>
#include <texrender/formula_layout.hpp>
#include <texrender/latex_document.hpp>

#ifndef TEXRENDER_RES_DIR
#define TEXRENDER_RES_DIR "res"
#endif

namespace
{
    constexpr auto k_background = Color{246, 247, 251, 255};
    constexpr auto k_panel = Color{255, 255, 255, 255};
    constexpr auto k_text = Color{40, 48, 62, 255};
    constexpr auto k_muted = Color{102, 112, 133, 255};
    constexpr auto k_real = Color{37, 99, 235, 255};
    constexpr auto k_imaginary = Color{190, 78, 125, 255};
    constexpr auto k_formula_color = std::uint32_t{0xff101820};
    constexpr auto k_pi = 3.14159265358979323846;
    constexpr auto k_animation_speed = 0.55;
    constexpr auto k_signal_frequency = 0.86;

    class RuntimeFormula
    {
    public:
        RuntimeFormula(const TeXRender::LatexDocument& document, const float text_size, const std::uint32_t color)
            : document_{document}
            , text_size_{text_size}
            , color_{color}
        {}

        void set_latex(std::string latex)
        {
            if (latex_ == latex)
                return;

            latex_ = std::move(latex);
            handle_ = document_.render(latex_, text_size_, color_);
        }

        void draw_centered(const int center_x, const int center_y) const
        {
            if (!handle_)
                return;

            handle_->draw(center_x - handle_->width() / 2, center_y - handle_->height() / 2);
        }

    private:
        const TeXRender::LatexDocument& document_;
        float text_size_ = 48.f;
        std::uint32_t color_ = 0xff000000;
        std::string latex_;
        std::optional<TeXRender::RenderHandle> handle_;
    };

    [[nodiscard]] std::string format_number(const double value)
    {
        auto buffer = std::array<char, 32>{};
        std::snprintf(buffer.data(), buffer.size(), "%.2f", value);
        return buffer.data();
    }

    [[nodiscard]] std::string format_abs_number(const double value)
    {
        return format_number(std::abs(value));
    }

    [[nodiscard]] std::string value_tex(const std::string& value)
    {
        return R"(\textcolor{#266c5b}{\mathrm{)" + value + R"(}})";
    }

    [[nodiscard]] std::string signed_value_tex(const double value)
    {
        return std::string{value < 0.0 ? "-" : "+"} + value_tex(format_abs_number(value));
    }

    [[nodiscard]] std::string imaginary_term_tex(const double value)
    {
        return std::string{value < 0.0 ? "-j" : "+j"} + value_tex(format_abs_number(value));
    }

    [[nodiscard]] std::string symbolic_complex_signal_tex()
    {
        return R"(x(t)=Ae^{j\left(2\pi\cdot f\cdot t+\alpha\right)})";
    }

    [[nodiscard]] std::string symbolic_euler_signal_tex()
    {
        return R"(=A\left[\cos\left(2\pi\cdot f\cdot t+\alpha\right)+j\sin\left(2\pi\cdot f\cdot t+\alpha\right)\right])";
    }

    [[nodiscard]] std::string variables_tex(const std::string& amplitude, const std::string& frequency, const std::string& time,
        const std::string& phase)
    {
        return R"(A=)" + value_tex(amplitude) + R"(,\quad f=)" + value_tex(frequency) + R"(\,\mathrm{Hz},\quad t=)" +
               value_tex(time) + R"(\,\mathrm{s},\quad \alpha=)" + value_tex(phase) + R"(\,\mathrm{rad})";
    }

    [[nodiscard]] std::string complex_signal_tex(const std::string& amplitude, const std::string& frequency,
        const std::string& time, const std::string& phase_term)
    {
        return R"(=)" + value_tex(amplitude) + R"(e^{j\left(2\pi\cdot)" + value_tex(frequency) + R"(\cdot)" + value_tex(time) +
               phase_term + R"(\right)})";
    }

    [[nodiscard]] std::string euler_signal_tex(const std::string& amplitude, const std::string& frequency, const std::string& time,
        const std::string& phase_term, const std::string& real, const std::string& imaginary_term)
    {
        const auto angle = R"(\left(2\pi\cdot)" + value_tex(frequency) + R"(\cdot)" + value_tex(time) + phase_term + R"(\right))";
        return R"(=)" + value_tex(amplitude) + R"(\left[\cos)" + angle + R"(+j\sin)" + angle + R"(\right]=)" + value_tex(real) +
               imaginary_term;
    }

    [[nodiscard]] double amplitude_at(const double clock)
    {
        return 42.0 + 6.0 * (std::sin(clock * 0.16) + 1.0) * 0.5;
    }

    [[nodiscard]] double frequency_at()
    {
        return k_signal_frequency;
    }

    [[nodiscard]] double phase_offset_at(const double clock)
    {
        return -0.9 + 1.8 * (std::sin(clock * 0.14 + 1.4) + 1.0) * 0.5;
    }

    [[nodiscard]] double signal_time_at(const double clock)
    {
        return clock * k_animation_speed;
    }

    void draw_signal_trace(const Rectangle area, const double amplitude, const double frequency, const double current_angle)
    {
        DrawRectangleRounded(area, 0.08f, 12, Color{248, 250, 252, 255});
        DrawRectangleLinesEx(area, 1.f, Color{226, 232, 240, 255});

        const auto center_y = area.y + area.height * 0.5f;
        DrawLineEx(Vector2{area.x + 12.f, center_y}, Vector2{area.x + area.width - 12.f, center_y}, 1.f, Color{203, 213, 225, 255});

        auto previous_real = Vector2{};
        auto previous_imaginary = Vector2{};
        constexpr auto samples = 220;
        for (auto i = 0; i < samples; ++i)
        {
            constexpr auto trace_seconds = 1.5;
            const auto progress = static_cast<double>(i) / static_cast<double>(samples - 1);
            const auto sample_offset = (progress - 0.5) * trace_seconds;
            const auto angle = current_angle + 2.0 * k_pi * frequency * sample_offset;
            const auto real = amplitude * std::cos(angle);
            const auto imaginary = amplitude * std::sin(angle);
            const auto x = area.x + 12.f + static_cast<float>(progress) * (area.width - 24.f);
            const auto scale = (area.height - 28.f) / static_cast<float>(2.0 * amplitude);
            const auto current_real = Vector2{x, center_y - static_cast<float>(real) * scale};
            const auto current_imaginary = Vector2{x, center_y - static_cast<float>(imaginary) * scale};

            if (i > 0)
            {
                DrawLineEx(previous_real, current_real, 2.5f, k_real);
                DrawLineEx(previous_imaginary, current_imaginary, 2.5f, k_imaginary);
            }

            previous_real = current_real;
            previous_imaginary = current_imaginary;
        }

        DrawRectangleRounded(Rectangle{area.x + 14.f, area.y + 8.f, 214.f, 24.f}, 0.3f, 8, Color{248, 250, 252, 245});
        DrawCircle(static_cast<int>(area.x + 28.f), static_cast<int>(area.y + 20.f), 5.f, k_real);
        DrawText("real / cos", static_cast<int>(area.x + 42.f), static_cast<int>(area.y + 12.f), 16, k_muted);
        DrawCircle(static_cast<int>(area.x + 134.f), static_cast<int>(area.y + 20.f), 5.f, k_imaginary);
        DrawText("imag / sin", static_cast<int>(area.x + 148.f), static_cast<int>(area.y + 12.f), 16, k_muted);
    }
} // namespace

int main()
{
    InitWindow(1180, 650, "TeXRender realtime values");
    SetTargetFPS(60);

    {
        const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR};

        auto symbolic_complex_formula = RuntimeFormula{document, 28.f, k_formula_color};
        auto complex_formula = RuntimeFormula{document, 34.f, k_formula_color};
        auto symbolic_euler_formula = RuntimeFormula{document, 25.f, k_formula_color};
        auto euler_formula = RuntimeFormula{document, 24.f, k_formula_color};
        auto variables_formula = RuntimeFormula{document, 23.f, k_formula_color};
        symbolic_complex_formula.set_latex(symbolic_complex_signal_tex());
        symbolic_euler_formula.set_latex(symbolic_euler_signal_tex());
        const auto draw_frame = [&](const double clock) {
            const auto amplitude = amplitude_at(clock);
            const auto frequency = frequency_at();
            const auto phase = phase_offset_at(clock);
            const auto time = signal_time_at(clock);
            const auto angle = 2.0 * k_pi * frequency * time + phase;
            const auto real = amplitude * std::cos(angle);
            const auto imaginary = amplitude * std::sin(angle);
            const auto amplitude_text = format_number(amplitude);
            const auto frequency_text = format_number(frequency);
            const auto phase_text = format_number(phase);
            const auto phase_term = signed_value_tex(phase);
            const auto time_text = format_number(time);
            const auto real_text = format_number(real);
            const auto imaginary_term = imaginary_term_tex(imaginary);
            complex_formula.set_latex(complex_signal_tex(amplitude_text, frequency_text, time_text, phase_term));
            euler_formula.set_latex(euler_signal_tex(amplitude_text, frequency_text, time_text, phase_term, real_text,
                imaginary_term));
            variables_formula.set_latex(variables_tex(amplitude_text, frequency_text, time_text, phase_text));
            ClearBackground(k_background);
            DrawText("Realtime complex signal", 32, 30, 28, k_text);
            DrawText("Signal variables, symbolic form, live substitution, and real/imaginary traces.", 32, 66, 18, k_muted);
            const auto panel = Rectangle{48.f, 118.f, static_cast<float>(GetScreenWidth() - 96), 520.f};
            DrawRectangleRounded(panel, 0.05f, 12, k_panel);
            DrawRectangleLinesEx(panel, 2.f, Color{218, 223, 230, 255});
            DrawText("variables", 86, 135, 16, k_muted);
            DrawText("model", 86, 206, 16, k_muted);
            DrawText("substitution", 86, 282, 16, k_muted);
            DrawText("Euler form", 86, 370, 16, k_muted);
            DrawText("result", 86, 445, 16, k_muted);
            DrawText("waveform", 86, 500, 16, k_muted);

            variables_formula.draw_centered(GetScreenWidth() / 2, 158);
            symbolic_complex_formula.draw_centered(GetScreenWidth() / 2, 228);
            complex_formula.draw_centered(GetScreenWidth() / 2, 303);
            symbolic_euler_formula.draw_centered(GetScreenWidth() / 2, 392);
            euler_formula.draw_centered(GetScreenWidth() / 2, 468);
            draw_signal_trace(Rectangle{86.f, 525.f, static_cast<float>(GetScreenWidth() - 172), 104.f}, amplitude, frequency,
                angle);
        };

        const auto export_screenshot = [&](const char* const path, const double clock) {
            const auto target = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
            BeginTextureMode(target);
            draw_frame(clock);
            EndTextureMode();
            auto image = LoadImageFromTexture(target.texture);
            ImageFlipVertical(&image);
            ExportImage(image, path);
            UnloadImage(image);
            UnloadRenderTexture(target);
        };

        if (const auto* const shot = std::getenv("TEXRENDER_SIGNAL_SCREENSHOT"))
        {
            auto capture_clock = 4.0;
            if (const auto* const clock_env = std::getenv("TEXRENDER_SIGNAL_CLOCK"))
            {
                char* end{};
                const auto parsed = std::strtod(clock_env, &end);
                if (end != clock_env)
                    capture_clock = parsed;
            }

            export_screenshot(shot, capture_clock);
        }
        else if (const auto* const shot = std::getenv("TEXRENDER_SCREENSHOT"))
        {
            export_screenshot(shot, 4.0);
        }
        else
        {
            while (!WindowShouldClose())
            {
                BeginDrawing();
                draw_frame(GetTime());
                EndDrawing();
            }
        }
    }

    CloseWindow();
    return 0;
}
