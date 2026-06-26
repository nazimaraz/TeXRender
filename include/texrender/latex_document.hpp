#pragma once

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

    struct RenderOptions
    {
        static constexpr auto no_input_limit = std::size_t{0};
        static constexpr auto default_untrusted_input_limit = std::size_t{64u * 1'024u};
        static constexpr auto no_text_size_limit = 0.f;
        static constexpr auto default_untrusted_max_text_size = 512.f;
        static constexpr auto no_wrap_width_limit = 0;
        static constexpr auto default_untrusted_max_wrap_width = 32'768;
        float text_size = 48.f;
        std::uint32_t argb = 0xff000000;
        int wrap_width = 0;
        std::size_t max_input_bytes = no_input_limit;
        float max_text_size = no_text_size_limit;
        int max_wrap_width = no_wrap_width_limit;
    };

    enum class RenderErrorCode : std::uint8_t
    {
        none,
        invalid_options,
        input_too_large,
        text_size_too_large,
        wrap_width_too_large,
        parse_error,
        unexpected_failure
    };

    struct RenderError
    {
        RenderErrorCode code = RenderErrorCode::none;
        std::string message;
    };

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

        [[nodiscard]] bool has_value() const noexcept
        {
            return handle_.has_value();
        }

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return has_value();
        }

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

        [[nodiscard]] RenderHandle* operator->()
        {
            return &value();
        }

        [[nodiscard]] const RenderHandle* operator->() const
        {
            return &value();
        }

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

        [[nodiscard]] const RenderError& error() const noexcept
        {
            return error_;
        }

        [[nodiscard]] std::optional<RenderHandle> into_optional() && noexcept
        {
            return std::move(handle_);
        }

    private:
        std::optional<RenderHandle> handle_;
        RenderError error_;
    };

    class TEXRENDER_EXPORT LatexDocument
    {
    public:
        explicit LatexDocument(const std::string& res_root, std::unique_ptr<IDrawSurface> surface = nullptr);
        LatexDocument(const LatexDocument&) = delete;
        LatexDocument(LatexDocument&&) = delete;
        LatexDocument& operator=(const LatexDocument&) = delete;
        LatexDocument& operator=(LatexDocument&&) = delete;
        ~LatexDocument() noexcept;

        [[nodiscard]] IDrawSurface& surface() const;
        [[nodiscard]] RenderHandle render(const std::string& latex, float text_size, std::uint32_t argb, int wrap_width = 0) const;
        [[nodiscard]] RenderHandle render(const std::string& latex, const RenderOptions& options) const;
        [[nodiscard]] std::optional<RenderHandle> try_render(const std::string& latex, float text_size, std::uint32_t argb,
            int wrap_width = 0) const noexcept;
        [[nodiscard]] std::optional<RenderHandle> try_render(const std::string& latex, const RenderOptions& options) const noexcept;
        [[nodiscard]] RenderResult try_render_detailed(const std::string& latex, float text_size, std::uint32_t argb,
            int wrap_width = 0) const noexcept;
        [[nodiscard]] RenderResult try_render_detailed(const std::string& latex, const RenderOptions& options) const noexcept;

    private:
        std::shared_ptr<detail::DocumentState> state_;
    };

    TEXRENDER_EXPORT void shutdown() noexcept;
} // namespace TeXRender
