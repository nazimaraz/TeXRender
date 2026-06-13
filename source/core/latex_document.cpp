#include "texgui/latex_document.hpp"
#include <cmath>
#include <exception>
#include <limits>
#include <stdexcept>
#include "core/document_state.hpp"
#include "latex.h"
#include "render/glyph_cache.hpp"
#include "utils/exceptions.h"

using namespace TexGui;

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

    void validate_options(const std::string& latex, const RenderOptions& options)
    {
        if (!std::isfinite(options.text_size) || options.text_size <= 0.f)
            throw std::invalid_argument("texgui: text_size must be finite and positive");

        if (options.max_input_bytes != RenderOptions::no_input_limit && latex.size() > options.max_input_bytes)
            throw std::length_error("texgui: LaTeX input exceeds max_input_bytes");

        if (options.max_text_size != RenderOptions::no_text_size_limit && options.text_size > options.max_text_size)
            throw std::length_error("texgui: text_size exceeds max_text_size");

        if (options.max_wrap_width != RenderOptions::no_wrap_width_limit && options.wrap_width > options.max_wrap_width)
            throw std::length_error("texgui: wrap_width exceeds max_wrap_width");
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
    validate_options(latex, options);
    const auto width = options.wrap_width > 0 ? options.wrap_width : std::numeric_limits<int>::max() / 2;
    const auto render = tex::LaTeX::parse(tex::utf82wide(latex), width, options.text_size, 0.f, options.argb);
    return {render, state_};
}

std::optional<RenderHandle> LatexDocument::try_render(const std::string& latex, const float text_size, const uint32_t argb,
    const int wrap_width) const noexcept
{
    return try_render(latex, untrusted_options(text_size, argb, wrap_width));
}

std::optional<RenderHandle> LatexDocument::try_render(const std::string& latex, const RenderOptions& options) const noexcept
{
    try
    {
        return render(latex, options);
    } catch (const tex::ex_tex&)
    {
        return std::nullopt; // malformed LaTeX — the documented nullopt case
    } catch (...)
    {
        // try_render exists for arbitrary caller input, so even a failure that
        // isn't a TeX error must not escape the noexcept boundary. Surface it
        // via TEXGUI_DEBUG=1 instead of crashing the caller.
        detail::debug_log("try_render: swallowed a non-TeX failure");
        return std::nullopt;
    }
}
