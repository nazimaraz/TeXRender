#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
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

    template <typename Fn>
    [[nodiscard]] double time_ms(Fn&& fn)
    {
        const auto start = Clock::now();
        fn();
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
} // namespace

int main()
{
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

        const auto render_ms = time_ms([&] {
            for (auto i = 0; i < iterations; ++i)
                (void) document.render(std::string{item.latex}, item.text_size, 0xff000000);
        });
        print_row(item.name, "render", render_ms, iterations);

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
        }
    }
}
