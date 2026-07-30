#include <texrender/headless_surface.hpp>
#include <texrender/latex_document.hpp>
#include <texrender/svg_surface.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifndef TEXRENDER_RES_DIR
#   define TEXRENDER_RES_DIR "res"
#endif

namespace
{
    using Clock = std::chrono::steady_clock;

    constexpr auto k_text_size = 42.f;
    constexpr auto k_argb = std::uint32_t{0xff101820};
    constexpr auto k_margin = 8;

    volatile std::uint64_t g_sink = 0;

    struct Summary
    {
        double median_ms{};
        double p95_ms{};
        double min_ms{};
        double max_ms{};
        std::size_t iterations{};
    };

    [[nodiscard]] const std::vector<std::string>& formulas()
    {
        static const auto values = std::vector<std::string>{
            R"(e^{i\pi}+1=0)",
            R"(\frac{a+b}{\sqrt{c}}+\sum_{n=1}^{5} n^2)",
            R"(\int_0^\infty e^{-x^2}\,dx=\frac{\sqrt{\pi}}{2})",
            R"(\nabla \cdot \vec{E} = \frac{\rho}{\epsilon_0})",
            R"(\left(\frac{x^2+y^2}{1+z}\right)^{3/2})",
            R"(\text{OpenMath benchmark})",
        };
        return values;
    }

    [[nodiscard]] double elapsed_ms(const Clock::time_point start, const Clock::time_point stop)
    {
        return std::chrono::duration<double, std::milli>{stop - start}.count();
    }

    template <typename Func>
    [[nodiscard]] std::vector<double> measure_samples(const std::size_t iterations, const std::size_t warmups, Func func)
    {
        for (auto i = std::size_t{0}; i < warmups; ++i)
            func();

        auto samples = std::vector<double>{};
        samples.reserve(iterations);
        for (auto i = std::size_t{0}; i < iterations; ++i)
        {
            const auto start = Clock::now();
            func();
            const auto stop = Clock::now();
            samples.push_back(elapsed_ms(start, stop));
        }

        return samples;
    }

    [[nodiscard]] Summary summarize(std::vector<double> samples)
    {
        std::sort(samples.begin(), samples.end());
        const auto percentile = [&](const double value) {
            const auto index = static_cast<std::size_t>(std::ceil(value * static_cast<double>(samples.size()))) - 1u;
            return samples[std::min(index, samples.size() - 1u)];
        };

        return Summary{
            percentile(0.50),
            percentile(0.95),
            samples.front(),
            samples.back(),
            samples.size(),
        };
    }

    template <typename Func>
    [[nodiscard]] Summary measure(const std::size_t iterations, const std::size_t warmups, Func func)
    {
        return summarize(measure_samples(iterations, warmups, std::move(func)));
    }

    void draw_headless(TeXRender::LatexDocument& document, TeXRender::HeadlessSurface& surface, const std::string& formula)
    {
        auto handle = document.render(formula, k_text_size, k_argb);
        surface.resize(handle.width() + 2 * k_margin, handle.height() + 2 * k_margin);
        surface.clear(0x00000000);
        handle.draw(k_margin, k_margin);
        g_sink += static_cast<std::uint64_t>(surface.covered_pixels());
    }

    void export_svg(TeXRender::LatexDocument& document, TeXRender::SvgSurface& surface, const std::string& formula)
    {
        auto handle = document.render(formula, k_text_size, k_argb);
        surface.resize(handle.width() + 2 * k_margin, handle.height() + 2 * k_margin);
        surface.clear(0x00000000);
        handle.draw(k_margin, k_margin);
        const auto svg = surface.svg();
        g_sink += static_cast<std::uint64_t>(svg.size());
    }

    [[nodiscard]] double measure_document_init_ms()
    {
        const auto start = Clock::now();
        auto surface = std::make_unique<TeXRender::HeadlessSurface>();
        const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(surface)};
        const auto stop = Clock::now();
        return elapsed_ms(start, stop);
    }

    [[nodiscard]] std::string compiler_version()
    {
#if defined(__clang__)
        return std::string{"Clang "} + __clang_version__;
#elif defined(__GNUC__)
        return std::string{"GCC "} + __VERSION__;
#elif defined(_MSC_VER)
        return "MSVC " + std::to_string(_MSC_VER);
#else
        return "unknown";
#endif
    }

    [[nodiscard]] std::string build_mode()
    {
#if defined(NDEBUG)
        return "Release";
#else
        return "Debug";
#endif
    }

    [[nodiscard]] std::string current_date()
    {
        const auto now = std::time(nullptr);
        auto tm = std::tm{};
#if defined(_WIN32)
        localtime_s(&tm, &now);
#else
        localtime_r(&now, &tm);
#endif
        auto stream = std::ostringstream{};
        stream << std::put_time(&tm, "%Y-%m-%d");
        return stream.str();
    }

    [[nodiscard]] std::string cpu_name()
    {
        auto file = std::ifstream{"/proc/cpuinfo"};
        auto line = std::string{};
        while (std::getline(file, line))
        {
            constexpr auto prefix = std::string_view{"model name"};
            if (line.compare(0, prefix.size(), prefix) != 0)
                continue;

            const auto colon = line.find(':');
            if (colon == std::string::npos)
                continue;

            auto value = line.substr(colon + 1);
            value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](const unsigned char ch) {
                return !std::isspace(ch);
            }));
            return value;
        }

        return "unknown";
    }

    void print_summary_row(const std::string_view scenario, const Summary& summary)
    {
        std::cout << "| " << scenario << " | " << std::fixed << std::setprecision(3) << summary.median_ms << " ms | "
                  << summary.p95_ms << " ms | " << summary.min_ms << " ms | " << summary.max_ms << " ms | " << summary.iterations
                  << " |\n";
    }
} // namespace

int main()
{
    try
    {
        const auto init_ms = measure_document_init_ms();

        auto headless_surface = std::make_unique<TeXRender::HeadlessSurface>();
        auto* const headless = headless_surface.get();
        auto headless_document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(headless_surface)};

        auto first_render_samples = std::vector<double>{};
        first_render_samples.reserve(formulas().size());
        for (const auto& formula : formulas())
        {
            const auto start = Clock::now();
            draw_headless(headless_document, *headless, formula);
            const auto stop = Clock::now();
            first_render_samples.push_back(elapsed_ms(start, stop));
        }

        auto formula_index = std::size_t{0};
        const auto warm_render = measure(600, 60, [&] {
            draw_headless(headless_document, *headless, formulas()[formula_index++ % formulas().size()]);
        });

        auto cached_handle = headless_document.render(formulas()[2], k_text_size, k_argb);
        headless->resize(cached_handle.width() + 2 * k_margin, cached_handle.height() + 2 * k_margin);
        const auto cached_draw = measure(3000, 200, [&] {
            headless->clear(0x00000000);
            cached_handle.draw(k_margin, k_margin);
            g_sink += static_cast<std::uint64_t>(headless->covered_pixels());
        });

        auto svg_surface = std::make_unique<TeXRender::SvgSurface>();
        auto* const svg = svg_surface.get();
        auto svg_document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(svg_surface)};
        formula_index = 0;
        const auto svg_export = measure(400, 40, [&] {
            export_svg(svg_document, *svg, formulas()[formula_index++ % formulas().size()]);
        });

        formula_index = 0;
        const auto batch_iterations = std::size_t{1000};
        const auto batch_start = Clock::now();
        for (auto i = std::size_t{0}; i < batch_iterations; ++i)
            draw_headless(headless_document, *headless, formulas()[formula_index++ % formulas().size()]);
        const auto batch_stop = Clock::now();
        const auto batch_ms = elapsed_ms(batch_start, batch_stop);
        const auto formulas_per_second = static_cast<double>(batch_iterations) / (batch_ms / 1000.0);

        std::cout << "## TeXRender Benchmark\n\n";
        std::cout << "- Date: " << current_date() << '\n';
        std::cout << "- Build: " << build_mode() << '\n';
        std::cout << "- Compiler: " << compiler_version() << '\n';
        std::cout << "- CPU: " << cpu_name() << '\n';
        std::cout << "- Backend: headless surface plus SVG surface\n\n";
        std::cout << "| Scenario | Median | p95 | Min | Max | Iterations |\n";
        std::cout << "|---|---:|---:|---:|---:|---:|\n";
        std::cout << "| document init | " << std::fixed << std::setprecision(3) << init_ms << " ms | " << init_ms << " ms | "
                  << init_ms << " ms | " << init_ms << " ms | 1 |\n";
        print_summary_row("first render+draw/headless", summarize(std::move(first_render_samples)));
        print_summary_row("warm render+draw/headless", warm_render);
        print_summary_row("cached draw+clear/headless", cached_draw);
        print_summary_row("render+draw+serialize/svg", svg_export);
        std::cout << "\nBatch render+draw/headless: " << std::fixed << std::setprecision(1) << formulas_per_second
                  << " formulas/s (" << batch_iterations << " formulas in " << batch_ms << " ms).\n";
        std::cout << "\n<!-- sink: " << g_sink << " -->\n";
        return 0;
    } catch (const std::exception& error)
    {
        std::cerr << "benchmark failed: " << error.what() << '\n';
        return 1;
    }
}
