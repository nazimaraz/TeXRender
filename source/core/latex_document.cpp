#include "texrender/latex_document.hpp"
#include <cmath>
#include <exception>
#include <limits>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>
#include "core/document_state.hpp"
#include "latex.h"
#include "render/glyph_cache.hpp"
#include "utils/exceptions.h"

using namespace TeXRender;

namespace
{
    [[nodiscard]] RenderOptions untrusted_options(const float text_size, const uint32_t argb, const int wrap_width)
    {
        auto options = RenderOptions{};
        options.text_size = text_size;
        options.argb = argb;
        options.wrap_width = wrap_width;
        options.max_input_bytes = RenderOptions::default_untrusted_input_limit;
        options.max_text_size = RenderOptions::default_untrusted_max_text_size;
        options.max_wrap_width = RenderOptions::default_untrusted_max_wrap_width;
        return options;
    }

    [[nodiscard]] std::optional<RenderError> validate_options(const std::string& latex, const RenderOptions& options)
    {
        if (!std::isfinite(options.text_size) || options.text_size <= 0.f)
            return RenderError{RenderErrorCode::invalid_options, "texrender: text_size must be finite and positive"};

        if (options.wrap_width < 0)
            return RenderError{RenderErrorCode::invalid_options, "texrender: wrap_width must be non-negative"};

        if (options.max_wrap_width < 0)
            return RenderError{RenderErrorCode::invalid_options, "texrender: max_wrap_width must be non-negative"};

        if (options.max_input_bytes != RenderOptions::no_input_limit && latex.size() > options.max_input_bytes)
            return RenderError{RenderErrorCode::input_too_large, "texrender: LaTeX input exceeds max_input_bytes"};

        if (options.max_text_size != RenderOptions::no_text_size_limit && options.text_size > options.max_text_size)
            return RenderError{RenderErrorCode::text_size_too_large, "texrender: text_size exceeds max_text_size"};

        if (options.max_wrap_width != RenderOptions::no_wrap_width_limit && options.wrap_width > options.max_wrap_width)
            return RenderError{RenderErrorCode::wrap_width_too_large, "texrender: wrap_width exceeds max_wrap_width"};

        return std::nullopt;
    }

    [[noreturn]] void throw_render_error(const RenderError& error)
    {
        switch (error.code)
        {
            case RenderErrorCode::input_too_large:
            case RenderErrorCode::text_size_too_large:
            case RenderErrorCode::wrap_width_too_large:
                throw std::length_error(error.message);
            case RenderErrorCode::invalid_options:
                throw std::invalid_argument(error.message);
            case RenderErrorCode::parse_error:
            case RenderErrorCode::unexpected_failure:
            case RenderErrorCode::none:
                throw std::runtime_error(error.message.empty() ? "texrender: render failed" : error.message);
        }

        throw std::runtime_error(error.message.empty() ? "texrender: render failed" : error.message);
    }

    [[nodiscard]] int effective_wrap_width(const RenderOptions& options)
    {
        if (options.wrap_width > 0)
            return options.wrap_width;

        if (options.max_wrap_width != RenderOptions::no_wrap_width_limit)
            return options.max_wrap_width;

        return std::numeric_limits<int>::max() / 2;
    }
} // namespace

LatexDocument::LatexDocument(const std::string& res_root, std::unique_ptr<IDrawSurface> surface)
    : state_{std::make_shared<detail::DocumentState>(res_root, std::move(surface))}
{}

LatexDocument::~LatexDocument() = default;

IDrawSurface& LatexDocument::surface() const
{
    return *state_->surface();
}

RenderHandle LatexDocument::render(const std::string& latex, const float text_size, const uint32_t argb, const int wrap_width) const
{
    auto options = RenderOptions{};
    options.text_size = text_size;
    options.argb = argb;
    options.wrap_width = wrap_width;
    return render(latex, options);
}

RenderHandle LatexDocument::render(const std::string& latex, const RenderOptions& options) const
{
    if (const auto error = validate_options(latex, options))
        throw_render_error(*error);

    const auto lock = std::scoped_lock{detail::render_mutex()};
    const auto width = effective_wrap_width(options);
    const auto render = tex::LaTeX::parse(tex::utf82wide(latex), width, options.text_size, 0.f, options.argb);
    return {render, state_};
}

std::optional<RenderHandle> LatexDocument::try_render(const std::string& latex, const float text_size, const uint32_t argb,
    const int wrap_width) const noexcept
{
    auto result = try_render_detailed(latex, text_size, argb, wrap_width);
    return std::move(result).into_optional();
}

std::optional<RenderHandle> LatexDocument::try_render(const std::string& latex, const RenderOptions& options) const noexcept
{
    auto result = try_render_detailed(latex, options);
    return std::move(result).into_optional();
}

RenderResult LatexDocument::try_render_detailed(const std::string& latex, const float text_size, const uint32_t argb,
    const int wrap_width) const noexcept
{
    return try_render_detailed(latex, untrusted_options(text_size, argb, wrap_width));
}

RenderResult LatexDocument::try_render_detailed(const std::string& latex, const RenderOptions& options) const noexcept
{
    try
    {
        if (const auto error = validate_options(latex, options))
            return RenderResult{*error};

        const auto lock = std::scoped_lock{detail::render_mutex()};
        const auto width = effective_wrap_width(options);
        auto* const parsed_render = tex::LaTeX::parse(tex::utf82wide(latex), width, options.text_size, 0.f, options.argb);
        return RenderResult{RenderHandle{parsed_render, state_}};
    } catch (const tex::ex_tex& error)
    {
        return RenderResult{RenderError{RenderErrorCode::parse_error, error.what()}};
    } catch (const std::exception& error)
    {
        detail::debug_log(std::string{"try_render_detailed: swallowed a non-TeX failure: "} + error.what());
        return RenderResult{RenderError{RenderErrorCode::unexpected_failure, error.what()}};
    } catch (...)
    {
        // try_render exists for arbitrary caller input, so even a failure that
        // isn't a TeX error must not escape the noexcept boundary. Surface it
        // via TEXRENDER_DEBUG=1 instead of crashing the caller.
        detail::debug_log("try_render_detailed: swallowed an unknown non-TeX failure");
        return RenderResult{RenderError{RenderErrorCode::unexpected_failure, "texrender: unknown render failure"}};
    }
}
