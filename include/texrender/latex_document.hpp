#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include "texrender/draw_surface.hpp"
#include "texrender/export.hpp"
#include "texrender/render_handle.hpp"

namespace TeXRender
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
    /// Colors use packed `0xAARRGGBB`; alpha is honored by headless/SVG output
    /// and mapped to the selected GUI backend's color model.
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
        ///
        /// Wrapping is a layout hint passed to MicroTeX. It does not clip output;
        /// callers should still size or clip their target surface as needed.
        int wrap_width = 0;
        /// Maximum accepted UTF-8 input size in bytes, or no_input_limit.
        std::size_t max_input_bytes = no_input_limit;
        /// Maximum accepted text size in pixels, or no_text_size_limit.
        float max_text_size = no_text_size_limit;
        /// Maximum accepted/effective wrap width in pixels, or no_wrap_width_limit.
        int max_wrap_width = no_wrap_width_limit;
    };

    /// Machine-readable reason for a non-throwing render failure.
    ///
    /// Use this with `try_render_detailed()` when UI code needs to distinguish a
    /// validation failure from a MicroTeX parse error without throwing through a
    /// frame loop or request handler.
    enum class RenderErrorCode
    {
        none,
        invalid_options,
        input_too_large,
        text_size_too_large,
        wrap_width_too_large,
        parse_error,
        unexpected_failure
    };

    /// Diagnostic payload returned by the detailed non-throwing render API.
    ///
    /// `message` is intended for logs or developer-facing diagnostics. It may
    /// include parser text from MicroTeX and should be sanitized/translated
    /// before showing it directly to untrusted end users.
    struct RenderError
    {
        RenderErrorCode code = RenderErrorCode::none;
        std::string message;
    };

    /// Result object for callers that need an error code/message instead of
    /// only std::optional.
    ///
    /// The object is intentionally small and move-friendly: on success it owns a
    /// `RenderHandle`; on failure `error()` describes why no handle was created.
    class RenderResult
    {
    public:
        RenderResult() = default;
        explicit RenderResult(RenderHandle handle)
            : handle_{std::move(handle)}
        {}
        explicit RenderResult(RenderError error)
            : error_{std::move(error)}
        {}

        /// Whether a render handle is present.
        [[nodiscard]] bool has_value() const noexcept
        {
            return handle_.has_value();
        }
        [[nodiscard]] explicit operator bool() const noexcept
        {
            return has_value();
        }
        /// Access the render handle after checking `has_value()`/operator bool.
        [[nodiscard]] RenderHandle& operator*() &
        {
            return value();
        }
        [[nodiscard]] const RenderHandle& operator*() const&
        {
            return value();
        }
        [[nodiscard]] RenderHandle&& operator*() &&
        {
            return std::move(*this).value();
        }
        /// Access members of the render handle after checking success.
        [[nodiscard]] RenderHandle* operator->()
        {
            return &value();
        }
        [[nodiscard]] const RenderHandle* operator->() const
        {
            return &value();
        }
        /// Return the render handle, throwing `std::bad_optional_access` if empty.
        [[nodiscard]] RenderHandle& value() &
        {
            return handle_.value();
        }
        [[nodiscard]] const RenderHandle& value() const&
        {
            return handle_.value();
        }
        [[nodiscard]] RenderHandle&& value() &&
        {
            return std::move(handle_).value();
        }
        /// Return the diagnostic payload. Meaningful when `has_value()` is false.
        [[nodiscard]] const RenderError& error() const noexcept
        {
            return error_;
        }
        /// Convert into the compact optional form, discarding any error details.
        [[nodiscard]] std::optional<RenderHandle> into_optional() && noexcept
        {
            return std::move(handle_);
        }

    private:
        std::optional<RenderHandle> handle_;
        RenderError error_;
    };

    /// Owner of MicroTeX document state and the drawing surface used by formulas.
    ///
    /// A document parses LaTeX into move-only RenderHandle objects. Documents
    /// are intentionally non-copyable and non-movable because MicroTeX has
    /// process-wide initialization and the selected surface often has graphics
    /// context lifetime requirements. Construct and use documents on the
    /// rendering thread. The first document in a process initializes MicroTeX's
    /// resource root; later documents should use the same `res_root`.
    class TEXRENDER_EXPORT LatexDocument
    {
    public:
        /// Construct a document using a MicroTeX resource directory.
        ///
        /// `res_root` must point to MicroTeX's runtime resource tree. When
        /// `surface` is null, texrender creates the default surface for the
        /// configured backend (`TEXRENDER_BACKEND`). Pass a custom IDrawSurface to
        /// render into another target, for example HeadlessSurface for offscreen
        /// RGBA output. GUI surfaces are borrowed from their toolkit context, so
        /// keep that context alive until documents and handles are destroyed.
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
        /// maximum effective width. The returned handle can be drawn repeatedly
        /// without reparsing the formula.
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
        /// Non-throwing render path that preserves a diagnostic failure reason.
        [[nodiscard]] RenderResult try_render_detailed(const std::string& latex, float text_size, uint32_t argb,
            int wrap_width = 0) const noexcept;
        /// Non-throwing render path with caller-provided guardrails and diagnostics.
        [[nodiscard]] RenderResult try_render_detailed(const std::string& latex, const RenderOptions& options) const noexcept;

    private:
        std::shared_ptr<detail::DocumentState> state_;
    };

    /// Compatibility hook for process-wide MicroTeX shutdown.
    ///
    /// Optional and idempotent. It is safe to call while LatexDocument or
    /// RenderHandle objects are still alive; in that case it does nothing. In
    /// the current release this function intentionally leaves MicroTeX's global
    /// parser/font tables alive for the rest of the process because the pinned
    /// upstream teardown path is not safe to re-initialize afterward.
    ///
    /// The shared FreeType glyph cache and MicroTeX state are released by normal
    /// process teardown.
    TEXRENDER_EXPORT void shutdown() noexcept;
} // namespace TeXRender
