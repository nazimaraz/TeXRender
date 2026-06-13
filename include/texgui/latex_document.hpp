#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include "texgui/draw_surface.hpp"
#include "texgui/render_handle.hpp"

namespace TexGui
{
    namespace detail
    {
        class DocumentState;
    }

    struct RenderOptions
    {
        static constexpr std::size_t no_input_limit = 0;
        static constexpr std::size_t default_untrusted_input_limit = 64u * 1024u;
        static constexpr float no_text_size_limit = 0.f;
        static constexpr float default_untrusted_max_text_size = 512.f;
        static constexpr int no_wrap_width_limit = 0;
        static constexpr int default_untrusted_max_wrap_width = 32768;

        float text_size = 48.f;
        uint32_t argb = 0xff000000;
        int wrap_width = 0;
        std::size_t max_input_bytes = no_input_limit;
        float max_text_size = no_text_size_limit;
        int max_wrap_width = no_wrap_width_limit;
    };

    class LatexDocument
    {
    public:
        // res_root is the MicroTeX resource directory (fonts + metrics).
        //
        // The document owns the surface its formulas draw into. By default that
        // is the surface of the backend selected at build time (TEXGUI_BACKEND);
        // pass your own IDrawSurface implementation to draw elsewhere — e.g. a
        // HeadlessSurface to render offscreen into an inspectable RGBA canvas.
        explicit LatexDocument(const std::string& res_root, std::unique_ptr<IDrawSurface> surface = nullptr);
        LatexDocument(const LatexDocument&) = delete;
        LatexDocument(LatexDocument&&) = delete;
        LatexDocument& operator=(const LatexDocument&) = delete;
        LatexDocument& operator=(LatexDocument&&) = delete;
        ~LatexDocument();

        // The surface this document's formulas draw into. Valid for the lifetime
        // of the document and of every RenderHandle it produced.
        [[nodiscard]] IDrawSurface& surface() const;

        // Parse and lay out latex at the given pixel text_size, tinted argb
        // (0xAARRGGBB). wrap_width > 0 wraps to that pixel width; 0 lays the
        // formula out on a single unbounded line.
        //
        // Throws a std::exception-derived error when the LaTeX is malformed.
        // Pass trusted input, or use try_render() for caller input that may not
        // parse.
        [[nodiscard]] RenderHandle render(const std::string& latex, float text_size, uint32_t argb, int wrap_width = 0) const;
        [[nodiscard]] RenderHandle render(const std::string& latex, const RenderOptions& options) const;

        // Non-throwing variant of render(): returns std::nullopt instead of
        // throwing when the LaTeX fails to parse or violates the default
        // untrusted-input limits.
        [[nodiscard]] std::optional<RenderHandle> try_render(const std::string& latex, float text_size, uint32_t argb,
            int wrap_width = 0) const noexcept;
        [[nodiscard]] std::optional<RenderHandle> try_render(const std::string& latex, const RenderOptions& options) const noexcept;

    private:
        std::shared_ptr<detail::DocumentState> state_;
    };
} // namespace TexGui
