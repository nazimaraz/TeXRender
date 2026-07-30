#include "test_helpers.hpp"
#include <memory>
#include <string>
#include "texrender/headless_surface.hpp"
#include "texrender/latex_document.hpp"

namespace
{
    [[nodiscard]] TeXRender::RenderOptions base_options()
    {
        auto options = TeXRender::RenderOptions{};
        options.text_size = 36.f;
        options.argb = 0xff000000;
        options.wrap_width = 512;
        return options;
    }

    [[nodiscard]] bool rejects_parse_error(const TeXRender::LatexDocument& document)
    {
        auto options = base_options();
        options.max_input_bytes = 16;
        return TeXRender::Test::expect_render_error(document, R"(a\\b)", options, TeXRender::RenderErrorCode::parse_error);
    }

    [[nodiscard]] bool rejects_invalid_utf8(const TeXRender::LatexDocument& document)
    {
        auto options = base_options();
        auto input = std::string{"x"};
        input.push_back(static_cast<char>(0xc0));
        input.push_back(static_cast<char>(0xaf));
        return TeXRender::Test::expect_render_error(document, input, options, TeXRender::RenderErrorCode::parse_error);
    }

    [[nodiscard]] bool rejects_text_size_limit(const TeXRender::LatexDocument& document)
    {
        auto options = base_options();
        options.text_size = 600.f;
        options.max_text_size = 512.f;
        return TeXRender::Test::expect_render_error(document, R"(\text{too large})", options,
            TeXRender::RenderErrorCode::text_size_too_large);
    }

    [[nodiscard]] bool rejects_input_limit(const TeXRender::LatexDocument& document)
    {
        auto options = base_options();
        options.max_input_bytes = 4;
        return TeXRender::Test::expect_render_error(document, R"(\frac{1}{2})", options,
            TeXRender::RenderErrorCode::input_too_large);
    }

    [[nodiscard]] bool rejects_wrap_limit(const TeXRender::LatexDocument& document)
    {
        auto options = base_options();
        options.wrap_width = 100000;
        options.max_wrap_width = 4096;
        return TeXRender::Test::expect_render_error(document, R"(\text{wrap})", options,
            TeXRender::RenderErrorCode::wrap_width_too_large);
    }

    [[nodiscard]] bool rejects_invalid_options(const TeXRender::LatexDocument& document)
    {
        auto options = base_options();
        options.text_size = 0.f;
        return TeXRender::Test::expect_render_error(document, R"(\text{invalid})", options,
            TeXRender::RenderErrorCode::invalid_options);
    }
} // namespace

int main()
{
    auto owned = std::make_unique<TeXRender::HeadlessSurface>();
    const auto document = TeXRender::LatexDocument{TEXRENDER_RES_DIR, std::move(owned)};

    auto ok = true;
    ok = TeXRender::Test::run_case("parse-error", rejects_parse_error(document)) && ok;
    ok = TeXRender::Test::run_case("invalid-utf8", rejects_invalid_utf8(document)) && ok;
    ok = TeXRender::Test::run_case("text-size-limit", rejects_text_size_limit(document)) && ok;
    ok = TeXRender::Test::run_case("input-limit", rejects_input_limit(document)) && ok;
    ok = TeXRender::Test::run_case("wrap-limit", rejects_wrap_limit(document)) && ok;
    ok = TeXRender::Test::run_case("invalid-options", rejects_invalid_options(document)) && ok;
    return ok ? 0 : 1;
}
