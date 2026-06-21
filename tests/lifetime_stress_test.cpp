#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include "texrender/cache_options.hpp"
#include "texrender/headless_surface.hpp"
#include "texrender/latex_document.hpp"

#ifndef TEXRENDER_RES_DIR
#define TEXRENDER_RES_DIR "res"
#endif

using namespace TeXRender;

namespace
{
    std::atomic<int> g_failures{0};

    struct StressConfig
    {
        int outlive_iterations = 50;
        int render_iterations = 100;
        int shutdown_iterations = 40;
        int worker_threads = 4;
        int thread_iterations = 20;
        int handles_per_document = 8;
    };

    void check(const bool ok, const char* const expr, const char* const file, const int line)
    {
        if (!ok)
        {
            std::printf("FAIL: %s:%d: %s\n", file, line, expr);
            g_failures.fetch_add(1, std::memory_order_relaxed);
        }
    }

#define CHECK(cond) check((cond), #cond, __FILE__, __LINE__)

    int env_int(const char* const name, const int fallback, const int min_value, const int max_value)
    {
        const auto* const value = std::getenv(name);
        if (!value || value[0] == '\0')
            return fallback;

        errno = 0;
        char* end = nullptr;
        const auto parsed = std::strtol(value, &end, 10);
        if (errno != 0 || end == value || *end != '\0' || parsed < min_value || parsed > max_value)
        {
            std::printf("invalid %s=%s; expected %d..%d, using %d\n", name, value, min_value, max_value, fallback);
            return fallback;
        }

        return static_cast<int>(parsed);
    }

    StressConfig load_config()
    {
        auto config = StressConfig{};
        config.outlive_iterations = env_int("TEXRENDER_LIFETIME_OUTLIVE_ITERATIONS", config.outlive_iterations, 1, 10000);
        config.render_iterations = env_int("TEXRENDER_LIFETIME_RENDER_ITERATIONS", config.render_iterations, 1, 10000);
        config.shutdown_iterations = env_int("TEXRENDER_LIFETIME_SHUTDOWN_ITERATIONS", config.shutdown_iterations, 1, 10000);
        config.worker_threads = env_int("TEXRENDER_LIFETIME_THREADS", config.worker_threads, 1, 64);
        config.thread_iterations = env_int("TEXRENDER_LIFETIME_THREAD_ITERATIONS", config.thread_iterations, 1, 10000);
        config.handles_per_document = env_int("TEXRENDER_LIFETIME_HANDLES_PER_DOCUMENT", config.handles_per_document, 1, 128);
        return config;
    }

    void draw_visible(RenderHandle& formula, HeadlessSurface& canvas, const int padding)
    {
        canvas.resize(std::max(1, formula.width() + padding * 2), std::max(1, formula.height() + padding * 2));
        canvas.clear(0x00000000);
        formula.draw(padding, padding);
    }

    void check_visible(RenderHandle& formula, HeadlessSurface& canvas, const int padding)
    {
        draw_visible(formula, canvas, padding);
        CHECK(canvas.covered_pixels() > 0);
    }

    std::string indexed_formula(const int index)
    {
        switch (index % 6)
        {
            case 0:
                return R"(\sum_{n=1}^{10} n^2)";
            case 1:
                return R"(\sqrt{x+1})";
            case 2:
                return R"(\frac{x+1}{y+1})";
            case 3:
                return R"(\int_0^1 x^2\,dx)";
            case 4:
                return R"(\left(\frac{a}{b}\right)^2)";
            default:
                return R"(e^{i\pi}+1=0)";
        }
    }

    void exercise_handle_outlives_document(const std::string& res, const StressConfig& config)
    {
        for (auto i = 0; i < config.outlive_iterations; ++i)
        {
            auto owned = std::make_unique<HeadlessSurface>();
            auto* const canvas = owned.get();
            auto formula = std::optional<RenderHandle>{};

            {
                const auto document = LatexDocument{res, std::move(owned)};
                formula.emplace(document.render(indexed_formula(i), 32.f, 0xff000000));
                CHECK(formula->width() > 0);
                CHECK(formula->height() > 0);
            }

            check_visible(*formula, *canvas, 10);
            formula.reset();
        }
    }

    void exercise_many_handles_outlive_document(const std::string& res, const StressConfig& config)
    {
        for (auto iteration = 0; iteration < config.outlive_iterations; ++iteration)
        {
            auto owned = std::make_unique<HeadlessSurface>();
            auto* const canvas = owned.get();
            auto formulas = std::vector<RenderHandle>{};
            formulas.reserve(static_cast<std::size_t>(config.handles_per_document));

            {
                const auto document = LatexDocument{res, std::move(owned)};
                for (auto i = 0; i < config.handles_per_document; ++i)
                    formulas.emplace_back(document.render(indexed_formula(iteration + i), 24.f + static_cast<float>(i % 5),
                        0xff000000));
            }

            std::reverse(formulas.begin(), formulas.end());
            for (auto& formula : formulas)
                check_visible(formula, *canvas, 6);

            if (!formulas.empty())
            {
                auto moved = std::optional<RenderHandle>{std::move(formulas.back())};
                formulas.pop_back();
                check_visible(*moved, *canvas, 6);
                formulas.emplace_back(std::move(*moved));
            }
        }
    }

    void exercise_try_render_result_lifetimes(const std::string& res, const StressConfig& config)
    {
        for (auto i = 0; i < config.render_iterations; ++i)
        {
            auto owned = std::make_unique<HeadlessSurface>();
            auto* const canvas = owned.get();
            const auto document = LatexDocument{res, std::move(owned)};
            auto result = document.try_render_detailed(indexed_formula(i), 28.f, 0xff000000);
            CHECK(result.has_value());
            if (result)
                check_visible(result.value(), *canvas, 6);
        }
    }

    void exercise_shutdown_interleavings(const std::string& res, const StressConfig& config)
    {
        for (auto i = 0; i < config.shutdown_iterations; ++i)
        {
            shutdown();

            auto surviving = std::optional<RenderHandle>{};
            auto* canvas = static_cast<HeadlessSurface*>(nullptr);
            {
                auto owned = std::make_unique<HeadlessSurface>();
                canvas = owned.get();
                const auto document = LatexDocument{res, std::move(owned)};
                shutdown();
                surviving.emplace(document.render(indexed_formula(i), 30.f, 0xff000000));
                shutdown();
            }

            CHECK(surviving.has_value());
            check_visible(*surviving, *canvas, 8);
            shutdown();
            surviving.reset();
            shutdown();
        }
    }

    void exercise_concurrent_try_render(const std::string& res, const StressConfig& config)
    {
        auto limits = cache_limits();
        limits.max_cpu_glyphs = 16;
        set_cache_limits(limits);

        auto threads = std::vector<std::thread>{};
        threads.reserve(static_cast<std::size_t>(config.worker_threads));
        for (auto worker = 0; worker < config.worker_threads; ++worker)
        {
            threads.emplace_back([&res, &config, worker] {
                for (auto i = 0; i < config.thread_iterations; ++i)
                {
                    if ((i % 7) == 0)
                        shutdown();

                    auto owned = std::make_unique<HeadlessSurface>();
                    auto* const canvas = owned.get();
                    const auto document = LatexDocument{res, std::move(owned)};
                    auto options = RenderOptions{};
                    options.text_size = 22.f + static_cast<float>((worker + i) % 8);
                    options.argb = 0xff000000;
                    options.max_input_bytes = 256;
                    options.max_text_size = 128.f;
                    options.max_wrap_width = 512;

                    if (auto formula = document.try_render(indexed_formula(worker + i), options))
                    {
                        check_visible(*formula, *canvas, 4);
                    }
                    else
                    {
                        CHECK(false);
                    }
                }
            });
        }

        for (auto& thread : threads)
            thread.join();

        limits.max_cpu_glyphs = CacheLimits::default_cpu_glyphs;
        set_cache_limits(limits);
    }
} // namespace

int main()
{
    const auto res = std::string{TEXRENDER_RES_DIR};
    const auto config = load_config();

    std::printf(
        "lifetime stress config: outlive=%d render=%d shutdown=%d threads=%d thread_iterations=%d handles_per_document=%d\n",
        config.outlive_iterations, config.render_iterations, config.shutdown_iterations, config.worker_threads,
        config.thread_iterations, config.handles_per_document
    );

    exercise_handle_outlives_document(res, config);
    exercise_many_handles_outlive_document(res, config);
    exercise_try_render_result_lifetimes(res, config);
    exercise_shutdown_interleavings(res, config);
    exercise_concurrent_try_render(res, config);

    if (g_failures.load(std::memory_order_relaxed) == 0)
        std::printf("all lifetime_stress tests passed\n");

    return g_failures.load(std::memory_order_relaxed) == 0 ? 0 : 1;
}
