#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "color.hh"
#include "diagnostic.hh"
#include "shared.hh"


namespace
{
    using namespace cchell::diagnostics;
    using namespace cchell;


    auto
    get_visible_lines(std::string_view text,
                      std::uint32_t    first_line,
                      std::uint32_t    last_line)
        -> std::vector<std::pair<std::uint32_t, std::string_view>>
    {
        std::vector<std::pair<std::uint32_t, std::string_view>> result;

        std::uint32_t line_num { 1 };

        for (auto part : text | std::views::split('\n'))
        {
            if (line_num > last_line) break;

            if (line_num >= first_line)
            {
                std::string_view sv { part.begin(),
                                      static_cast<std::size_t>(
                                          std::ranges::distance(part)) };

                result.emplace_back(line_num, sv);
            }

            line_num++;
        }

        return result;
    }


    auto
    split_at(std::string_view s, std::size_t index)
        -> std::pair<std::string_view, std::string_view>
    {
        index = std::min(index, s.length());
        return { s.substr(0, index), s.substr(index) };
    }


    auto
    get_digits_amount(std::uint32_t n) -> std::uint8_t
    {
        /* a uint32 have a maximum value of 4,294,967,295
           which is only 10 digits. Which is not that
           difficult to hardcode */
        if (n >= 1'000'000'000) return 10;
        if (n >= 100'000'000) return 9;
        if (n >= 10'000'000) return 8;
        if (n >= 1'000'000) return 7;
        if (n >= 100'000) return 6;
        if (n >= 10'000) return 5;
        if (n >= 1'000) return 4;
        if (n >= 100) return 3;
        if (n >= 10) return 2;
        return 1;
    }


    auto
    severity_to_string(severity sev) -> std::string_view
    {
        switch (sev)
        {
        case severity::error:   return "error";
        case severity::warning: return "warning";
        case severity::note:    return "note";
        }
    }


    auto
    colorize(std::string_view text, cchell::color color, bool reset = true)
        -> std::string
    {
        return reset
                 ? std::format("{}{}{}", color, text, cchell::color::reset())
                 : std::format("{}{}", color, text);
    }
}


using cchell::diagnostics::diagnostic_builder;


diagnostic_builder::diagnostic_builder(severity lvl) { m_diag.level = lvl; }


auto
diagnostic_builder::domain(std::string_view domain) -> diagnostic_builder &&
{
    m_diag.domain = domain;
    return std::move(*this);
}


auto
diagnostic_builder::source(source_location source) -> diagnostic_builder &&
{
    m_diag.source = source;
    return std::move(*this);
}


auto
diagnostic_builder::length(std::size_t length) -> diagnostic_builder &&
{
    m_diag.length = length;
    return std::move(*this);
}


[[nodiscard]]
auto
diagnostic_builder::build() && -> diagnostic
{
    return std::move(m_diag);
}


auto
diagnostic::render(std::string_view raw_string,
                   std::string_view input_file,
                   const theme     &theme) -> std::string
{
    m_rendered.clear();
    m_padding = 0;

    shared::tty_status.stderr() ? render_colored(raw_string, input_file, theme)
                                : render_colorless(input_file);

    return m_rendered;
}


void
diagnostic::render_colored(std::string_view raw_string,
                           std::string_view input_file,
                           const theme     &theme)
{
    const std::uint32_t error_line { source.line + 1 };

    const std::uint32_t first_line { error_line > theme.extra_shown_line
                                         ? error_line - theme.extra_shown_line
                                         : 1 };

    const std::uint32_t last_line { error_line + theme.extra_shown_line };

    auto lines { get_visible_lines(raw_string, first_line, last_line) };

    m_line_number_width = get_digits_amount(last_line) + 1; /* + padding */

    render_header(theme);
    std::string colorless_source { create_colorless_source(input_file) };

    /* to calculate the padding, we need to get the max length of all
       the lines, which includes the source line. */
    for (const auto &[_, line] : lines)
        m_padding = std::max(line.length(), m_padding);

    /* and then we add the max length to the padding and the line number
       width, which normalizes the padding width */
    m_padding = std::max(m_padding, colorless_source.length())
              + theme.right_padding + m_line_number_width;

    render_source(colorless_source, theme);

    for (const auto &[i, line] : lines)
        render_line(i, line, i == error_line, theme);

    render_annotation(theme);
}


void
diagnostic::render_colorless(std::string_view input_file)
{
    std::string_view severity { severity_to_string(level) };

    m_rendered = std::format(
        /* clang-format off */
R"({} in {} at {}:{}:{}
  {}
  {}
)",
        /* clang-format on */
        severity, domain, input_file, source.line, source.column, message,
        annotation);
}


void
diagnostic::render_header(const theme &theme)
{
    std::string_view severity { severity_to_string(level) };

    if (domain.empty())
        m_rendered = std::format("{}{}{}: {}\n",
                                 theme.tag_color[std::to_underlying(level)],
                                 severity, color::reset(), message);
    else
        m_rendered = std::format("{}{}{} at {}{}{}: {}\n",
                                 theme.tag_color[std::to_underlying(level)],
                                 severity, color::reset(), theme.domain_color,
                                 domain, color::reset(), message);
}


auto
diagnostic::format_line(std::uint32_t    line_num,
                        std::string_view line,
                        std::size_t      line_len,
                        const theme     &theme) const -> std::string
{
    color       bg { 0, 0, 0 };
    std::string line_num_string;


    if (line_num == std::numeric_limits<std::uint32_t>::max())
    {
        bg              = theme.alt_line_color;
        line_num_string = std::string(m_line_number_width, ' ');
    }
    else
    {
        bg = line_num % 2 != 0 ? theme.line_color : theme.alt_line_color;
        line_num_string
            = std::string(m_line_number_width - get_digits_amount(line_num),
                          ' ')
            + std::to_string(line_num);
    }

    std::size_t pad { m_padding > line_len ? m_padding - line_len : 0 };

    return std::format(
        "{:ansi_bg}{}{} {}{}| {}{}{}{}\n", bg, theme.line_number_color,
        line_num_string, color::reset_attributes(), theme.separator_color,
        color::reset_attributes(), line, std::string(pad, ' '), color::reset());
}


auto
diagnostic::create_colorless_source(std::string_view input_file) const
    -> std::string
{
    return std::format("/* at {}:{}:{} */", input_file, source.line + 1,
                       source.column + 1);
}


void
diagnostic::render_source(std::string_view colorless_source, const theme &theme)
{
    m_rendered
        += format_line(std::numeric_limits<std::uint32_t>::max(),
                       colorize(colorless_source, theme.source_color, false),
                       colorless_source.length(), theme);
}


void
diagnostic::render_line(std::uint32_t    line_num,
                        std::string_view line,
                        bool             error_line,
                        const theme     &theme)
{
    std::string colored_line;

    if (error_line)
    {
        auto [left, rest] { split_at(line, source.column) };
        auto [error, tail] { split_at(rest, length) };

        colored_line = std::format(
            "{}{}{}{}{}{}{}{}", theme.code_color, left,
            color::reset_attributes(), theme.error_code_color, error,
            color::reset_attributes(), theme.code_color, tail);
    }
    else
        colored_line = colorize(line, theme.code_color, false);

    m_rendered += format_line(line_num, colored_line, line.length(), theme);
}


void
diagnostic::render_annotation(const theme &theme)
{
    std::size_t prefix { m_line_number_width + 3 };

    m_rendered.append(prefix + source.column, ' ');
    m_rendered += std::format("{}", theme.underline_color);
    m_rendered.append(length, '^');
    m_rendered += color::reset();
    m_rendered += '\n';

    m_rendered.append(prefix + source.column, ' ');
    m_rendered += annotation;
    m_rendered += '\n';
}
