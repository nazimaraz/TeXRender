#include "texrender/latex_document.hpp"
#include <cmath>
#include <exception>
#include <array>
#include <limits>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>
#include "core/document_state.hpp"
#include "microtex.h"
#include "render/glyph_cache.hpp"
#include "utils/exceptions.h"

using namespace TeXRender;

namespace
{
    [[nodiscard]] RenderOptions untrusted_options(const float text_size, const std::uint32_t argb, const int wrap_width)
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

    [[nodiscard]] bool is_continuation_byte(const unsigned char byte)
    {
        return (byte & 0xc0u) == 0x80u;
    }

    struct Utf8Sequence
    {
        unsigned char first_min;
        unsigned char first_max;
        unsigned char second_min;
        unsigned char second_max;
        std::size_t length;
    };

    [[nodiscard]] unsigned char input_byte_at(const std::string& input, const std::size_t index)
    {
        return static_cast<unsigned char>(input[index]);
    }

    [[nodiscard]] bool byte_in_range(const unsigned char byte, const unsigned char min, const unsigned char max)
    {
        return byte >= min && byte <= max;
    }

    [[nodiscard]] bool matches_utf8_sequence(const std::string& input, const std::size_t index, const Utf8Sequence& sequence)
    {
        if (input.size() - index < sequence.length)
            return false;

        if (!byte_in_range(input_byte_at(input, index + 1), sequence.second_min, sequence.second_max))
            return false;

        for (auto offset = std::size_t{2}; offset < sequence.length; ++offset)
        {
            if (!is_continuation_byte(input_byte_at(input, index + offset)))
                return false;
        }

        return true;
    }

    [[nodiscard]] bool is_valid_utf8(const std::string& input)
    {
        static constexpr auto multibyte_sequences = std::array{
            Utf8Sequence{0xc2u, 0xdfu, 0x80u, 0xbfu, 2},
            Utf8Sequence{0xe0u, 0xe0u, 0xa0u, 0xbfu, 3},
            Utf8Sequence{0xe1u, 0xecu, 0x80u, 0xbfu, 3},
            Utf8Sequence{0xedu, 0xedu, 0x80u, 0x9fu, 3},
            Utf8Sequence{0xeeu, 0xefu, 0x80u, 0xbfu, 3},
            Utf8Sequence{0xf0u, 0xf0u, 0x90u, 0xbfu, 4},
            Utf8Sequence{0xf1u, 0xf3u, 0x80u, 0xbfu, 4},
            Utf8Sequence{0xf4u, 0xf4u, 0x80u, 0x8fu, 4},
        };

        for (auto index = std::size_t{0}; index < input.size();)
        {
            const auto byte = input_byte_at(input, index);
            if (byte <= 0x7fu)
            {
                ++index;
                continue;
            }

            auto matched = false;
            for (const auto& sequence : multibyte_sequences)
            {
                if (!byte_in_range(byte, sequence.first_min, sequence.first_max))
                    continue;

                if (!matches_utf8_sequence(input, index, sequence))
                    return false;

                index += sequence.length;
                matched = true;
                break;
            }

            if (!matched)
                return false;
        }

        return true;
    }

    [[nodiscard]] bool has_unsafe_row_break(const std::string& input)
    {
        const auto has_environment = input.find(R"(\begin)") != std::string::npos;
        for (auto index = std::size_t{0}; index + 1 < input.size(); ++index)
        {
            if (input[index] != '\\' || input[index + 1] != '\\')
                continue;

            if (!has_environment)
                return true;

            auto next = index + 2;
            while (next < input.size() && (input[next] == ' ' || input[next] == '\t' || input[next] == '\n' || input[next] == '\r'))
                ++next;

            if (index == 0 || next >= input.size() || input[next] == '{' || input[next] == '}')
                return true;

            if (input[next] == '\\' && next + 1 < input.size() && (input[next + 1] == '[' || input[next + 1] == '('))
                return true;
        }

        return false;
    }

    [[nodiscard]] bool has_unsupported_alignment_mark(const std::string& input)
    {
        return input.find('&') != std::string::npos && input.find(R"(\begin)") == std::string::npos;
    }

    [[nodiscard]] std::optional<RenderError> validate_options(const std::string& latex, const RenderOptions& options)
    {
        if (!std::isfinite(options.text_size) || options.text_size <= 0.f)
            return RenderError{RenderErrorCode::invalid_options, "texrender: text_size must be finite and positive"};

        if (options.wrap_width < 0)
            return RenderError{RenderErrorCode::invalid_options, "texrender: wrap_width must be non-negative"};

        if (options.max_wrap_width < 0)
            return RenderError{RenderErrorCode::invalid_options, "texrender: max_wrap_width must be non-negative"};

        if (!std::isfinite(options.max_text_size) || options.max_text_size < 0.f)
            return RenderError{RenderErrorCode::invalid_options, "texrender: max_text_size must be finite and non-negative"};

        if (options.max_input_bytes != RenderOptions::no_input_limit && latex.size() > options.max_input_bytes)
            return RenderError{RenderErrorCode::input_too_large, "texrender: LaTeX input exceeds max_input_bytes"};

        if (options.max_text_size != RenderOptions::no_text_size_limit && options.text_size > options.max_text_size)
            return RenderError{RenderErrorCode::text_size_too_large, "texrender: text_size exceeds max_text_size"};

        if (options.max_wrap_width != RenderOptions::no_wrap_width_limit && options.wrap_width > options.max_wrap_width)
            return RenderError{RenderErrorCode::wrap_width_too_large, "texrender: wrap_width exceeds max_wrap_width"};

        for (const auto ch : latex)
        {
            if (const auto byte = static_cast<unsigned char>(ch);
                (byte < 0x20u && byte != '\t' && byte != '\n' && byte != '\r') || byte == 0x7fu)
            {
                return RenderError{
                    RenderErrorCode::parse_error, "texrender: LaTeX input contains an unsupported control character"
                };
            }
        }

        if (!is_valid_utf8(latex))
            return RenderError{RenderErrorCode::parse_error, "texrender: LaTeX input is not valid UTF-8"};

        if (has_unsafe_row_break(latex))
            return RenderError{RenderErrorCode::parse_error, "texrender: LaTeX row break is not in a supported context"};

        if (has_unsupported_alignment_mark(latex))
            return RenderError{RenderErrorCode::parse_error, "texrender: LaTeX alignment mark is not in a supported context"};

        if (latex.find('@') != std::string::npos)
            return RenderError{RenderErrorCode::parse_error, "texrender: LaTeX internal command character is not supported"};

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

    [[nodiscard]] microtex::Render* parse_latex(const std::string& latex, const RenderOptions& options)
    {
        const auto fill_width = latex.rfind("$$", 0) != 0 && latex.rfind(R"(\[)", 0) != 0;
        const auto line_space = options.wrap_width > 0 ? options.text_size * 0.25f : 0.f;
        return microtex::MicroTeX::parse(latex, static_cast<float>(effective_wrap_width(options)), options.text_size, line_space,
            options.argb, fill_width, {true, microtex::TexStyle::display});
    }
} // namespace

LatexDocument::LatexDocument(const std::string& res_root, std::unique_ptr<IDrawSurface> surface)
    : state_{std::make_shared<detail::DocumentState>(res_root, std::move(surface))}
{}

LatexDocument::~LatexDocument() noexcept = default;

IDrawSurface& LatexDocument::surface() const
{
    return *state_->surface();
}

RenderHandle LatexDocument::render(const std::string& latex, const float text_size, const std::uint32_t argb,
    const int wrap_width) const
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
    const auto render = parse_latex(latex, options);
    return {render, state_};
}

std::optional<RenderHandle> LatexDocument::try_render(const std::string& latex, const float text_size, const std::uint32_t argb,
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

RenderResult LatexDocument::try_render_detailed(const std::string& latex, const float text_size, const std::uint32_t argb,
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
        auto* const parsed_render = parse_latex(latex, options);
        return RenderResult{RenderHandle{parsed_render, state_}};
    } catch (const microtex::ex_tex& error)
    {
        return RenderResult{RenderError{RenderErrorCode::parse_error, error.what()}};
    } catch (const std::exception& error)
    {
        detail::debug_log(std::string{"try_render_detailed: swallowed a non-TeX failure: "} + error.what());
        return RenderResult{RenderError{RenderErrorCode::unexpected_failure, error.what()}};
    } catch (...)
    {
        detail::debug_log("try_render_detailed: swallowed an unknown non-TeX failure");
        return RenderResult{RenderError{RenderErrorCode::unexpected_failure, "texrender: unknown render failure"}};
    }
}
