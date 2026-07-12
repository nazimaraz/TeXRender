#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include "texrender/headless_surface.hpp"
#include "texrender/latex_document.hpp"

namespace TeXRender::Test
{
    [[nodiscard]] inline bool run_case(const char* name, const bool passed)
    {
        if (passed)
            return true;

        std::fprintf(stderr, "case failed: %s\n", name);
        return false;
    }

    [[nodiscard]] inline std::size_t count_occurrences(const std::string& value, const std::string_view needle)
    {
        auto count = std::size_t{0};
        auto offset = std::size_t{0};
        while ((offset = value.find(needle, offset)) != std::string::npos)
        {
            ++count;
            offset += needle.size();
        }

        return count;
    }

    [[nodiscard]] inline bool render_non_empty(const LatexDocument& document, HeadlessSurface& canvas, const std::string_view latex)
    {
        auto result = document.try_render_detailed(std::string{latex}, 36.f, 0xff000000, 512);
        if (!result)
        {
            std::fprintf(stderr, "render failed: %s\n", result.error().message.c_str());
            return false;
        }

        constexpr auto margin = 8;
        canvas.resize(result->width() + 2 * margin, result->height() + 2 * margin);
        canvas.clear(0x00000000);
        result->draw(margin, margin);

        if (result->width() <= 0 || result->height() <= 0 || canvas.covered_pixels() == 0)
        {
            std::fprintf(stderr, "empty render for: %.*s\n", static_cast<int>(latex.size()), latex.data());
            return false;
        }

        return true;
    }

    [[nodiscard]] inline bool expect_render_error(const LatexDocument& document, const std::string_view latex,
        const RenderOptions& options, const RenderErrorCode expected_code)
    {
        if (const auto result = document.try_render_detailed(std::string{latex}, options);
            result || result.error().code != expected_code)
        {
            std::fprintf(stderr, "expected render error %u for: %.*s\n", static_cast<unsigned>(expected_code),
                static_cast<int>(latex.size()), latex.data());
            return false;
        }

        return true;
    }
} // namespace TeXRender::Test
