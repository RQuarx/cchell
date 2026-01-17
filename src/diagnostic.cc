#include <algorithm>
#include <cmath>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "color.hh"
#include "diagnostic.hh"

using cchell::diagnostic;


namespace
{
    constexpr const std::size_t SHOWN_LINE { 2 };
    constexpr const std::size_t RIGHT_PADDING { 5 };


    [[nodiscard]]
    constexpr auto
    message_level_to_string(cchell::message_level level) -> std::string
    {
        switch (level)
        {
        case cchell::message_level::error:   return "error";
        case cchell::message_level::warning: return "warning";
        case cchell::message_level::note:    return "note";
        }
    }


    [[nodiscard]]
    auto
    split_lines(std::string_view text) -> std::vector<std::string_view>
    {
        std::vector<std::string_view> lines;
        std::size_t                   start { 0 };

        while (start < text.size())
        {
            std::size_t end { text.find('\n', start) };
            lines.emplace_back(text.substr(start, end - start));

            if (end == std::string_view::npos) break;
            start = end + 1;
        }

        return lines;
    }


    [[nodiscard]]
    auto
    split_at(std::string_view s, std::size_t index)
        -> std::pair<std::string_view, std::string_view>
    {
        index = std::min(index, s.length());
        return { s.substr(0, index), s.substr(index) };
    }


    [[nodiscard]]
    auto
    get_line_number_width(std::size_t line_number) -> std::size_t
    {
        return std::log10(line_number) + 1;
    }


    [[nodiscard]]
    auto
    format_line_number(std::size_t i,
                       std::size_t line_number_width,
                       bool        source) -> std::string
    {
        if (source) return std::string(line_number_width, ' '); /* NOLINT */

        return std::string(line_number_width - get_line_number_width(i), ' ')
             + std::to_string(i);
    }


    [[nodiscard]]
    auto
    format_line(std::size_t      i,
                std::string_view line,
                std::size_t      pad,
                std::size_t      line_number_width,
                bool             source = false) -> std::string
    {
        using namespace cchell;

        auto bg { i % 2 != 0 ? config::diagnostic.line_color
                             : config::diagnostic.alt_line_color };

        return std::format("{:ansi_bg}{}{} {}{}| {}{}{}{}\n", bg,
                           config::diagnostic.line_number_color,
                           format_line_number(i, line_number_width, source),
                           color::reset_attributes(),
                           config::diagnostic.separator_color,
                           color::reset_attributes(), line,
                           std::string(pad, ' '), color::reset());
    }


    [[nodiscard]]
    auto
    highlight_error(std::string_view line,
                    std::size_t      column,
                    std::size_t      length) -> std::string
    {
        auto [left, rest] { split_at(line, column) };
        auto [error, tail] { split_at(rest, length) };

        using namespace cchell;

        return std::format(
            "{}{}{}{}{}{}{}{}{}", config::diagnostic.code_color, left,
            color::reset_attributes(), config::diagnostic.error_code_color,
            error, color::reset_attributes(), config::diagnostic.code_color,
            tail, color::reset_attributes());
    }


    [[nodiscard]]
    auto
    calculate_padding(const std::vector<std::string_view> &lines,
                      std::size_t                          first_line,
                      std::size_t                          last_line,
                      std::string_view source_info) -> std::size_t
    {
        std::size_t pad { 0 };

        for (auto i : std::views::iota(first_line - 1, last_line))
            pad = std::max(lines[i].length(), pad);

        pad = std::max(pad, source_info.length());
        return pad + RIGHT_PADDING;
    }


    [[nodiscard]]
    auto
    colorize(std::string_view text, cchell::color color, bool reset = true)
        -> std::string
    {
        if (reset)
            return std::format("{}{}{}", color, text, cchell::color::reset());
        return std::format("{}{}", color, text,
                           cchell::color::reset_attributes());
    }


    void
    append_underline(std::string &string, std::size_t pos, std::size_t len)
    {
        string.append(pos, ' ');
        string += std::format("{}", cchell::config::diagnostic.underline_color);
        string.append(len, '^');
        string += cchell::color::reset();
        string += '\n';
    }
}


auto
diagnostic::format_message() -> std::string
{
    if (domain.empty())
        return std::format(
            "{}{}{}: {}",
            config::diagnostic.tag_color[std::to_underlying(level)],
            message_level_to_string(level), color::reset(), message);
    return std::format("{}{}{} at {}{}{}: {}",
                       config::diagnostic.tag_color[std::to_underlying(level)],
                       message_level_to_string(level), color::reset(),
                       config::diagnostic.domain_color, domain, color::reset(),
                       message);
}


auto
diagnostic::format() -> std::string
{
    /* 1. format header */
    std::string formatted { format_message() + '\n' };

    auto lines { split_lines(raw) };

    std::size_t first_line { source.line() > SHOWN_LINE
                                 ? source.line() - SHOWN_LINE
                                 : 1 };
    std::size_t last_line { std::min<std::size_t>(lines.size(),
                                                  source.line() + 1) };
    std::size_t line_number_width { get_line_number_width(last_line) + 1 };


    std::string source_info { std::format(
        "/* at {}:{}:{} */", file, source.line() + 1, source.column() + 1) };

    /* 2. compute padding */
    std::size_t padding { calculate_padding(lines, first_line, last_line,
                                            source_info)
                          + line_number_width };

    /* 3. add source information line */
    formatted += format_line(
        0, colorize(source_info, config::diagnostic.source_color, false),
        padding - source_info.length(), line_number_width, true);

    /* 4. format each code line */
    for (auto i : std::views::iota(first_line, last_line + 1))
    {
        const auto &line_content { lines[i - 1] };
        std::size_t pad { padding > line_content.size()
                              ? padding - line_content.size()
                              : 0 };

        std::string code_line;

        if (i == last_line)
            code_line = highlight_error(line_content, source.column(), length);
        else
            code_line = colorize(line_content, config::diagnostic.code_color);

        formatted += format_line(i, code_line, pad, line_number_width);
    }

    /* 5. add underline and annotation */
    std::size_t prefix { line_number_width + 3 };

    append_underline(formatted, prefix + source.column(), length);

    formatted.append(prefix + source.column(), ' ');
    formatted += annotation;
    formatted += '\n';

    return formatted;
}


auto
diagnostic::set_domain(std::string_view domain) -> diagnostic &
{
    this->domain = domain;
    return *this;
}


auto
diagnostic::set_raw(std::string_view raw) -> diagnostic &
{
    this->raw = raw;
    return *this;
}


auto
diagnostic::set_file(std::string_view file) -> diagnostic &
{
    this->file = file;
    return *this;
}


auto
diagnostic::set_source(source_location source) -> diagnostic &
{
    this->source = source;
    return *this;
}


auto
diagnostic::set_length(std::size_t length) -> diagnostic &
{
    this->length = length;
    return *this;
}


auto
diagnostic::set_level(message_level level) -> diagnostic &
{
    this->level = level;
    return *this;
}
