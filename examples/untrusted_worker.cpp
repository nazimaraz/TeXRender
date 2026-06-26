#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <texrender/headless_surface.hpp>
#include <texrender/latex_document.hpp>

#ifndef TEXRENDER_RES_DIR
#   define TEXRENDER_RES_DIR "res"
#endif

namespace
{
    constexpr auto k_max_input_bytes = std::size_t{8192};
    constexpr auto k_max_output_pixels = 2048;

    enum class ExitCode : std::uint8_t
    {
        success = 0,
        usage = 1,
        input_too_large = 2,
        render_error = 3,
        write_error = 4,
        output_too_large = 5,
        unexpected_error = 6
    };

    struct StdinReadResult
    {
        std::string input;
        bool too_large = false;
    };

    [[nodiscard]] StdinReadResult read_stdin_limited()
    {
        auto result = StdinReadResult{};
        result.input.reserve(k_max_input_bytes);

        for (auto ch = char{}; std::cin.get(ch);)
        {
            if (result.input.size() == k_max_input_bytes)
            {
                result.too_large = true;
                return result;
            }
            result.input.push_back(ch);
        }

        return result;
    }

    [[nodiscard]] TeXRender::RenderOptions worker_options()
    {
        auto options = TeXRender::RenderOptions{};
        options.text_size = 48.f;
        options.argb = 0xff111111;
        options.wrap_width = 960;
        options.max_input_bytes = k_max_input_bytes;
        options.max_text_size = 96.f;
        options.max_wrap_width = 960;
        return options;
    }

    [[nodiscard]] bool output_fits_worker_budget(const TeXRender::RenderHandle& formula)
    {
        return formula.width() > 0 && formula.height() > 0 && formula.width() <= k_max_output_pixels &&
               formula.height() <= k_max_output_pixels;
    }
} // namespace

int main(const int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "usage: texrender_untrusted_worker output.png < formula.tex\n";
        return static_cast<int>(ExitCode::usage);
    }

    try
    {
        const auto read_result = read_stdin_limited();
        if (read_result.too_large)
        {
            std::cerr << "input_too_large\n";
            return static_cast<int>(ExitCode::input_too_large);
        }

        auto owned = std::make_unique<TeXRender::HeadlessSurface>();
        auto* const canvas = owned.get();
        const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};

        auto result = document.try_render_detailed(read_result.input, worker_options());
        if (!result)
        {
            std::cerr << "render_error " << static_cast<int>(result.error().code) << '\n';
            return static_cast<int>(ExitCode::render_error);
        }

        if (!output_fits_worker_budget(*result))
        {
            std::cerr << "output_too_large\n";
            return static_cast<int>(ExitCode::output_too_large);
        }

        constexpr auto margin = 20;
        canvas->resize(result->width() + 2 * margin, result->height() + 2 * margin);
        canvas->clear(0xffffffff);
        result->draw(margin, margin);

        if (!canvas->write_png(argv[1]))
        {
            std::cerr << "write_error\n";
            return static_cast<int>(ExitCode::write_error);
        }

        std::cout << "ok\n";
        return static_cast<int>(ExitCode::success);
    } catch (const std::exception& error)
    {
        std::cerr << "unexpected_error: " << error.what() << '\n';
        return static_cast<int>(ExitCode::unexpected_error);
    }
}
