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

    /// Rendering options plus optional guardrails for caller-supplied input.
    ///
    /// `render()` treats unset limits as trusted-input mode. The simple
    /// `try_render(latex, text_size, argb, wrap_width)` overload fills these
    /// limits with conservative defaults before parsing arbitrary user text.
    struct RenderOptions
    {
        /// Sentinel that disables the input byte limit.
        static constexpr std::size_t no_input_limit = 0;
        /// Default maximum UTF-8 input size used by the simple try_render overload.
        static constexpr std::size_t default_untrusted_input_limit = 64u * 1024u;
        /// Sentinel that disables the text size limit.
        static constexpr float no_text_size_limit = 0.f;
        /// Default maximum pixel text size used by the simple try_render overload.
        static constexpr float default_untrusted_max_text_size = 512.f;
        /// Sentinel that disables the wrap width limit.
        static constexpr int no_wrap_width_limit = 0;
        /// Default maximum layout width used by the simple try_render overload.
        static constexpr int default_untrusted_max_wrap_width = 32768;

        /// Formula text size in pixels.
        float text_size = 48.f;
        /// Foreground color as 0xAARRGGBB.
        uint32_t argb = 0xff000000;
        /// Positive pixel width wraps layout; zero means unwrapped unless max_wrap_width is set.
        int wrap_width = 0;
        /// Maximum accepted UTF-8 input size in bytes, or no_input_limit.
        std::size_t max_input_bytes = no_input_limit;
        /// Maximum accepted text size in pixels, or no_text_size_limit.
        float max_text_size = no_text_size_limit;
        /// Maximum accepted/effective wrap width in pixels, or no_wrap_width_limit.
        int max_wrap_width = no_wrap_width_limit;
    };

    /// Owner of MicroTeX document state and the drawing surface used by formulas.
    ///
    /// A document parses LaTeX into move-only RenderHandle objects. Documents
    /// are intentionally non-copyable and non-movable because MicroTeX has
    /// process-wide initialization and the selected surface often has graphics
    /// context lifetime requirements. Construct and use documents on the
    /// rendering thread.
    class LatexDocument
    {
    public:
        /// Construct a document using a MicroTeX resource directory.
        ///
        /// `res_root` must point to MicroTeX's runtime resource tree. When
        /// `surface` is null, texgui creates the default surface for the
        /// configured backend (`TEXGUI_BACKEND`). Pass a custom IDrawSurface to
        /// render into another target, for example HeadlessSurface for offscreen
        /// RGBA output.
        explicit LatexDocument(const std::string& res_root, std::unique_ptr<IDrawSurface> surface = nullptr);
        LatexDocument(const LatexDocument&) = delete;
        LatexDocument(LatexDocument&&) = delete;
        LatexDocument& operator=(const LatexDocument&) = delete;
        LatexDocument& operator=(LatexDocument&&) = delete;
        ~LatexDocument();

        /// Surface this document's formulas draw into.
        ///
        /// The reference remains valid for the lifetime of the document and of
        /// every RenderHandle it produced.
        [[nodiscard]] IDrawSurface& surface() const;

        /// Parse and lay out trusted LaTeX input.
        ///
        /// Throws a std::exception-derived error when parsing fails or the
        /// supplied options are invalid. `wrap_width > 0` wraps to that pixel
        /// width; zero lays out an unbounded line unless RenderOptions sets a
        /// maximum effective width.
        [[nodiscard]] RenderHandle render(const std::string& latex, float text_size, uint32_t argb, int wrap_width = 0) const;
        /// Parse and lay out trusted LaTeX input with explicit options.
        [[nodiscard]] RenderHandle render(const std::string& latex, const RenderOptions& options) const;

        /// Non-throwing render path for caller-supplied LaTeX input.
        ///
        /// Returns std::nullopt when parsing fails or the default untrusted-input
        /// limits are exceeded. The method is noexcept and catches MicroTeX
        /// parse failures at the boundary.
        [[nodiscard]] std::optional<RenderHandle> try_render(const std::string& latex, float text_size, uint32_t argb,
            int wrap_width = 0) const noexcept;
        /// Non-throwing render path with caller-provided guardrails.
        [[nodiscard]] std::optional<RenderHandle> try_render(const std::string& latex, const RenderOptions& options) const noexcept;

    private:
        std::shared_ptr<detail::DocumentState> state_;
    };
} // namespace TexGui
