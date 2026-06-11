#include "texgui/latex_document.hpp"
#include <exception>
#include <limits>
#include "core/document_state.hpp"
#include "latex.h"
#include "render/glyph_cache.hpp"
#include "utils/exceptions.h"

using namespace TexGui;

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
    const auto width = wrap_width > 0 ? wrap_width : std::numeric_limits<int>::max() / 2;
    const auto render = tex::LaTeX::parse(tex::utf82wide(latex), width, text_size, 0.f, argb);
    return {render, state_};
}

std::optional<RenderHandle> LatexDocument::try_render(const std::string& latex, const float text_size, const uint32_t argb,
    const int wrap_width) const noexcept
{
    try
    {
        return render(latex, text_size, argb, wrap_width);
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
