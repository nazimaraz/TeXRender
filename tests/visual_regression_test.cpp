#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "texrender/headless_surface.hpp"
#include "texrender/latex_document.hpp"

#ifndef TEXRENDER_RES_DIR
#define TEXRENDER_RES_DIR "res"
#endif

#ifndef TEXRENDER_VISUAL_BASELINE_DIR
#define TEXRENDER_VISUAL_BASELINE_DIR "."
#endif

using namespace TeXRender;

namespace
{
    int g_failures = 0;

    void check(const bool ok, const char* const expr)
    {
        if (!ok)
        {
            std::printf("FAIL: %s\n", expr);
            ++g_failures;
        }
    }

    struct Bounds
    {
        int min_x = 0;
        int min_y = 0;
        int max_x = -1;
        int max_y = -1;
    };

    struct Baseline
    {
        std::string formula;
        float text_size = 56.f;
        int margin = 12;
        size_t min_covered_pixels = 150;
        int min_ink_width = 30;
        int min_ink_height = 25;
        int min_rule_run = 12;
        bool require_horizontal_rule = true;
    };

    [[nodiscard]] std::optional<Baseline> load_baseline(const std::filesystem::path& path)
    {
        auto file = std::ifstream{path};
        if (!file)
            return std::nullopt;

        auto baseline = Baseline{};
        auto line = std::string{};
        while (std::getline(file, line))
        {
            if (line.empty() || line.front() == '#')
                continue;

            const auto separator = line.find('=');
            if (separator == std::string::npos)
                continue;

            const auto key = std::string_view{line}.substr(0, separator);
            const auto value = line.substr(separator + 1);
            if (key == "formula")
                baseline.formula = value;
            else if (key == "text_size")
                baseline.text_size = std::stof(value);
            else if (key == "margin")
                baseline.margin = std::stoi(value);
            else if (key == "min_covered_pixels")
                baseline.min_covered_pixels = static_cast<size_t>(std::stoull(value));
            else if (key == "min_ink_width")
                baseline.min_ink_width = std::stoi(value);
            else if (key == "min_ink_height")
                baseline.min_ink_height = std::stoi(value);
            else if (key == "min_rule_run")
                baseline.min_rule_run = std::stoi(value);
            else if (key == "require_horizontal_rule")
                baseline.require_horizontal_rule = value == "true" || value == "1";
        }

        if (baseline.formula.empty())
            return std::nullopt;

        return baseline;
    }

    [[nodiscard]] Bounds ink_bounds(const HeadlessSurface& canvas)
    {
        auto bounds = Bounds{};
        auto first = true;
        const auto& pixels = canvas.pixels();
        for (auto y = 0; y < canvas.height(); ++y)
        {
            for (auto x = 0; x < canvas.width(); ++x)
            {
                const auto idx =
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(canvas.width()) + static_cast<std::size_t>(x)) * 4u;
                if (pixels[idx + 3] == 0)
                    continue;

                if (first)
                {
                    bounds = {x, y, x, y};
                    first = false;
                }
                else
                {
                    bounds.min_x = std::min(bounds.min_x, x);
                    bounds.min_y = std::min(bounds.min_y, y);
                    bounds.max_x = std::max(bounds.max_x, x);
                    bounds.max_y = std::max(bounds.max_y, y);
                }
            }
        }

        return bounds;
    }

    [[nodiscard]] bool has_horizontal_rule(const HeadlessSurface& canvas, const int min_run)
    {
        const auto& pixels = canvas.pixels();
        for (auto y = 0; y < canvas.height(); ++y)
        {
            auto run = 0;
            for (auto x = 0; x < canvas.width(); ++x)
            {
                const auto idx =
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(canvas.width()) + static_cast<std::size_t>(x)) * 4u;
                run = pixels[idx + 3] != 0 ? run + 1 : 0;
                if (run >= min_run)
                    return true;
            }
        }

        return false;
    }

    void write_artifacts(const std::string_view output_dir, const std::string_view name, const HeadlessSurface& canvas,
        const Bounds& bounds)
    {
        if (output_dir.empty())
            return;

        std::filesystem::create_directories(output_dir);
        const auto dir = std::filesystem::path{output_dir};
        const auto stem = std::string{name};
        const auto wrote_png = canvas.write_png((dir / (stem + ".png")).string());
        const auto wrote_ppm = canvas.write_ppm((dir / (stem + ".ppm")).string());

        auto metrics = std::ofstream{dir / (stem + ".metrics.txt")};
        metrics << "wrote_png=" << (wrote_png ? "true" : "false") << '\n';
        metrics << "wrote_ppm=" << (wrote_ppm ? "true" : "false") << '\n';
        metrics << "width=" << canvas.width() << '\n';
        metrics << "height=" << canvas.height() << '\n';
        metrics << "covered_pixels=" << canvas.covered_pixels() << '\n';
        metrics << "ink_min_x=" << bounds.min_x << '\n';
        metrics << "ink_min_y=" << bounds.min_y << '\n';
        metrics << "ink_max_x=" << bounds.max_x << '\n';
        metrics << "ink_max_y=" << bounds.max_y << '\n';
    }

    [[nodiscard]] bool env_flag(const char* const name)
    {
        const auto* const value = std::getenv(name);
        if (value == nullptr)
            return false;

        const auto text = std::string_view{value};
        return text == "1" || text == "true" || text == "TRUE" || text == "on" || text == "ON";
    }

    [[nodiscard]] std::vector<char> read_binary_file(const std::filesystem::path& path)
    {
        auto file = std::ifstream{path, std::ios::binary};
        return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    }

    void verify_snapshot(const std::string_view snapshot_dir, const bool update_snapshots, const std::string_view name,
        const HeadlessSurface& canvas)
    {
        if (snapshot_dir.empty())
            return;

        const auto expected = std::filesystem::path{snapshot_dir} / (std::string{name} + ".png");
        if (update_snapshots)
        {
            std::filesystem::create_directories(expected.parent_path());
            check(canvas.write_png(expected.string()), "canvas.write_png(expected.string())");
            return;
        }

        check(std::filesystem::exists(expected), "std::filesystem::exists(expected)");
        if (!std::filesystem::exists(expected))
            return;

        const auto actual = std::filesystem::temp_directory_path() / ("texrender-" + std::string{name} + "-visual-snapshot.png");
        check(canvas.write_png(actual.string()), "canvas.write_png(actual.string())");
        check(read_binary_file(actual) == read_binary_file(expected), "read_binary_file(actual) == read_binary_file(expected)");
        std::filesystem::remove(actual);
    }
} // namespace

#define CHECK(cond) check((cond), #cond)

int main()
{
    auto baseline_paths = std::vector<std::filesystem::path>{};
    for (const auto& entry : std::filesystem::directory_iterator{TEXRENDER_VISUAL_BASELINE_DIR})
    {
        if (entry.path().extension() == ".baseline")
            baseline_paths.push_back(entry.path());
    }
    std::sort(baseline_paths.begin(), baseline_paths.end());
    CHECK(!baseline_paths.empty());
    if (baseline_paths.empty())
        return 1;

    for (const auto& path : baseline_paths)
    {
        const auto baseline = load_baseline(path);
        CHECK(baseline.has_value());
        if (!baseline)
            continue;

        auto owned = std::make_unique<HeadlessSurface>();
        auto* const canvas = owned.get();
        const auto document = LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};
        const auto formula = document.render(baseline->formula, baseline->text_size, 0xff000000);

        const auto margin = baseline->margin;
        canvas->resize(formula.width() + 2 * margin, formula.height() + 2 * margin);
        canvas->clear(0x00000000);
        formula.draw(margin, margin);

        const auto bounds = ink_bounds(*canvas);
        const auto name = path.stem().string();
        const auto* const output_dir = std::getenv("TEXRENDER_VISUAL_OUTPUT_DIR");
        write_artifacts(output_dir != nullptr ? output_dir : "", name, *canvas, bounds);

        const auto* const snapshot_dir = std::getenv("TEXRENDER_VISUAL_SNAPSHOT_DIR");
        verify_snapshot(snapshot_dir != nullptr ? snapshot_dir : "", env_flag("TEXRENDER_VISUAL_UPDATE_SNAPSHOTS"), name, *canvas);

        CHECK(canvas->covered_pixels() > baseline->min_covered_pixels);
        CHECK(bounds.min_x >= margin - 4);
        CHECK(bounds.min_y >= margin - 4);
        CHECK(bounds.max_x < canvas->width() - margin + 6);
        CHECK(bounds.max_y < canvas->height() - margin + 6);
        CHECK(bounds.max_x - bounds.min_x > baseline->min_ink_width);
        CHECK(bounds.max_y - bounds.min_y > baseline->min_ink_height);
        if (baseline->require_horizontal_rule)
            CHECK(has_horizontal_rule(*canvas, std::max(baseline->min_rule_run, formula.width() / 3)));
    }

    if (g_failures == 0)
        std::printf("all visual_regression tests passed\n");

    return g_failures == 0 ? 0 : 1;
}
