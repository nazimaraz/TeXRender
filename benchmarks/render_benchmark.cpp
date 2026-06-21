#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <texrender/headless_surface.hpp>
#include <texrender/latex_document.hpp>

#ifndef TEXRENDER_RES_DIR
#define TEXRENDER_RES_DIR "res"
#endif

namespace
{
    using Clock = std::chrono::steady_clock;

    struct Case
    {
        std::string_view name;
        std::string_view latex;
        float text_size = 48.f;
    };

    struct Result
    {
        std::string_view name;
        std::string_view metric;
        double total_ms = 0.0;
        int iterations = 1;
    };

    template <typename Fn>
    [[nodiscard]] double time_ms(Fn&& fn)
    {
        const auto start = Clock::now();
        std::forward<Fn>(fn)();
        const auto end = Clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }

    void print_row(const std::string_view name, const std::string_view metric, const double ms, const int iterations)
    {
        std::cout << std::left << std::setw(18) << name << std::setw(18) << metric << std::right << std::fixed
                  << std::setprecision(3) << ms << " ms";
        if (iterations > 1)
            std::cout << " total, " << (ms / static_cast<double>(iterations)) << " ms/op";

        std::cout << '\n';
    }

    void write_json_string(std::ostream& out, const std::string_view value)
    {
        out << '"';
        for (const auto ch : value)
        {
            switch (ch)
            {
                case '\\':
                    out << "\\\\";
                    break;
                case '"':
                    out << "\\\"";
                    break;
                case '\n':
                    out << "\\n";
                    break;
                case '\r':
                    out << "\\r";
                    break;
                case '\t':
                    out << "\\t";
                    break;
                default:
                    out << ch;
                    break;
            }
        }
        out << '"';
    }

    bool write_json(const std::string& path, const std::vector<Result>& results)
    {
        auto out = std::ofstream{path};
        if (!out)
            return false;

        out << std::fixed << std::setprecision(6);
        out << "{\n";
        out << "  \"schema\": 1,\n";
        out << "  \"unit\": \"ms\",\n";
        out << "  \"metrics\": {\n";

        auto first_case = true;
        for (auto case_index = std::size_t{0}; case_index < results.size(); ++case_index)
        {
            const auto& result = results[case_index];
            if (case_index > 0 && result.name == results[case_index - 1].name)
                continue;

            if (!first_case)
                out << ",\n";
            first_case = false;

            out << "    ";
            write_json_string(out, result.name);
            out << ": {\n";

            auto first_metric = true;
            for (const auto& metric : results)
            {
                if (metric.name != result.name)
                    continue;

                if (!first_metric)
                    out << ",\n";
                first_metric = false;

                out << "      ";
                write_json_string(out, metric.metric);
                out << ": {\"total_ms\": " << metric.total_ms << ", \"iterations\": " << metric.iterations
                    << ", \"ms_per_op\": " << (metric.total_ms / static_cast<double>(metric.iterations)) << "}";
            }

            out << "\n    }";
        }

        out << "\n  }\n";
        out << "}\n";
        return static_cast<bool>(out);
    }
} // namespace

int main(const int argc, char** argv)
{
    auto json_path = std::string{};
    for (auto i = 1; i < argc; ++i)
    {
        const auto arg = std::string_view{argv[i]};
        if (arg == "--help")
        {
            std::cout << "usage: texrender_render_benchmark [--json <path>]\n";
            return 0;
        }
        if (arg == "--json")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "--json requires an output path\n";
                return 2;
            }
            json_path = argv[++i];
            continue;
        }

        std::cerr << "unknown argument: " << arg << '\n';
        return 2;
    }

    const auto cases = std::vector<Case>{
        {"short", R"(e^{i\pi} + 1 = 0)", 48.f},
        {"fraction", R"(\sqrt{\frac{a^2+b^2}{c}})", 56.f},
        {"long", R"(\sum_{i=1}^{n} \frac{\alpha_i}{1 + \beta_i^2} = \int_0^\infty e^{-x^2}\,dx)", 44.f},
    };

    auto owned = std::make_unique<TeXRender::HeadlessSurface>();
    auto* const canvas = owned.get();
    const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};

    std::cout << "texrender render benchmark\n";
    std::cout << std::left << std::setw(18) << "case" << std::setw(18) << "metric" << "time\n";

    auto results = std::vector<Result>{};
    for (const auto& item : cases)
    {
        constexpr auto iterations = 200;
        TeXRender::RenderHandle* first = nullptr;
        auto cold = std::optional<TeXRender::RenderHandle>{};
        const auto cold_ms = time_ms([&] {
            cold = document.render(std::string{item.latex}, item.text_size, 0xff000000);
            first = &*cold;
        });
        print_row(item.name, "cold render", cold_ms, 1);
        results.push_back(Result{item.name, "cold render", cold_ms, 1});

        const auto render_ms = time_ms([&] {
            for (auto i = 0; i < iterations; ++i)
                (void) document.render(std::string{item.latex}, item.text_size, 0xff000000);
        });
        print_row(item.name, "render", render_ms, iterations);
        results.push_back(Result{item.name, "render", render_ms, iterations});

        if (first)
        {
            canvas->resize(first->width() + 32, first->height() + 32);
            const auto draw_ms = time_ms([&] {
                for (auto i = 0; i < iterations; ++i)
                {
                    canvas->clear(0x00000000);
                    first->draw(16, 16);
                }
            });
            print_row(item.name, "draw headless", draw_ms, iterations);
            results.push_back(Result{item.name, "draw headless", draw_ms, iterations});
        }
    }

    if (!json_path.empty() && !write_json(json_path, results))
    {
        std::cerr << "could not write benchmark JSON: " << json_path << '\n';
        return 1;
    }

    return 0;
}
