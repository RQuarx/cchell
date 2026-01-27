#include <cstddef>
#include <limits>
#include <ranges>
#include <stack>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "lexer.hh"

using cchell::diagnostics::diagnostic_builder;
using cchell::diagnostics::severity;


namespace
{
    enum class quote_type : char
    {
        none    = '\0',
        single  = '\'',
        double_ = '"',
    };


    auto
    operator==(quote_type a, char b) -> bool
    {
        if (a == quote_type::single) return b == '\'';
        if (a == quote_type::double_) return b == '"';
        return true;
    }


    [[nodiscard]]
    auto
    is_punct(char c) -> bool
    {
        return std::string_view { "!$%^&*(){}[]|;:<>,?" }.find(c)
            != std::string_view::npos;
    }


    [[nodiscard]]
    constexpr auto
    matching_open(char c) -> char
    {
        switch (c)
        {
        case ')': return '(';
        case '}': return '{';
        case ']': return '[';
        default:  return '\0';
        }
    }


    constexpr auto
    is_quote(char c) -> quote_type
    {
        if (c == '\'') return quote_type::single;
        if (c == '"') return quote_type::double_;
        return quote_type::none;
    }


    auto
    decode_escape(char c) -> char
    {
        switch (c)
        {
        case 'n':  return '\n';
        case 't':  return '\t';
        case 'r':  return '\r';
        case '\\': return '\\';
        case '"':  return '"';
        default:   return c;
        }
    }


    auto
    find_next_whitespace(std::string_view string, std::size_t pos)
        -> std::size_t
    {
        for (auto i : std::views::iota(pos, string.length()))
            if (std::isspace(string[i]) != 0)
            {
                std::uint8_t backslash_count { 0 };

                for (std::size_t j { i - 1 }; j >= 0 && string[j] == '\\'; j--)
                    backslash_count++;

                if (backslash_count % 2 == 0) return i;
            }

        return std::string_view::npos;
    }


    [[nodiscard]]
    auto
    get_punct_token_type(char c) -> cchell::lexer::token_type
    {
        using cchell::lexer::token_type;

        switch (c)
        {
        case '(': [[fallthrough]];
        case ')': [[fallthrough]];
        case '{': [[fallthrough]];
        case '}': [[fallthrough]];
        case '[': [[fallthrough]];
        case ']': return token_type::bracket;

        case '|': return token_type::pipe;

        case '$': return token_type::dollar;

        default: return token_type::none;
        }
    }


    auto
    get_tokens_from_word(std::string_view                   word,
                         std::size_t                        index,
                         std::size_t                        line_start_index,
                         cchell::source_location            source,
                         std::vector<cchell::lexer::token> &out) -> std::size_t
    {
        using namespace cchell::lexer;

        std::size_t start { 0 };

        for (std::size_t i { 0 }; i < word.length(); i++)
        {
            if (is_quote(word[i]) != quote_type::none) return i;
            if (!is_punct(word[i])) continue;

            if (i > start)
            {
                source.column = (index + start) - line_start_index;
                out.emplace_back(token_type::word,
                                 word.substr(start, i - start), source);
            }

            source.column = (index + i) - line_start_index;
            out.emplace_back(get_punct_token_type(word[i]), word.substr(i, 1),
                             source);

            start = i + 1;
        }

        if (start < word.length())
        {
            source.column = (index + start) - line_start_index;
            out.emplace_back(token_type::word, word.substr(start), source);
        }

        return std::numeric_limits<std::size_t>::max();
    }


    auto
    get_tokens_from_string(std::string_view                   string,
                           std::size_t                       &index,
                           std::size_t                       &line_start_index,
                           cchell::source_location           &source,
                           std::vector<cchell::lexer::token> &tokens)
        -> std::uint8_t
    {
        using cchell::lexer::token_type;
        if (is_quote(string[index]) == quote_type::none) return 0;

        tokens.emplace_back(token_type::quote, string.substr(index, 1), source);
        source.column++;

        std::size_t closing_quote { string.find(string[index], index + 1) };
        if (closing_quote == std::string::npos)
        {
            tokens.emplace_back(token_type::word, string.substr(index + 1),
                                source);
            return 2;
        }

        std::size_t len { closing_quote - (index + 1) };

        tokens.emplace_back(token_type::word, string.substr(index + 1, len),
                            source);


        for (std::size_t i { index + 1 }; i < closing_quote; i++)
        {
            if (string[i] == '\n')
            {
                source.line++;
                source.column    = 0;
                line_start_index = ++index;
            }
            else
                source.column++;
        }


        tokens.emplace_back(token_type::quote, string.substr(closing_quote, 1),
                            source);
        source.column++;

        index = closing_quote + 1;
        return 1;
    }
}


auto
cchell::lexer::lex(std::string_view string) -> std::vector<token>
{
    std::vector<token> tokens;
    tokens.reserve(string.size() / 2);

    std::size_t      index { 0 };
    std::size_t      line_start_index { 0 };
    source_location  source;
    bool             escaped { false };

    while (index < string.length())
    {
        char c { string[index] };

        if (!escaped && c == '\n')
        {
            source.line++;
            source.column    = 0;
            line_start_index = ++index;

            continue;
        }

        if (!escaped && c == '\\')
        {
            escaped = true;
            index++;
            continue;
        }

        if (std::uint8_t res { get_tokens_from_string(
                string, index, line_start_index, source, tokens) };
            res == 1)
            continue;
        else if (res == 2) /* NOLINT: Do not use 'else' after 'return' */
            return tokens;

        if (!escaped && std::isspace(c) != 0)
        {
            index++;
            source.column = index - line_start_index;
        }

        std::size_t next_whitespace { find_next_whitespace(string, index + 1) };
        if (next_whitespace == std::string_view::npos)
            next_whitespace = string.length();

        std::string_view word { string.data() + index,
                                next_whitespace - index };

        std::size_t skip { get_tokens_from_word(word, index, line_start_index,
                                                source, tokens) };

        if (skip != std::numeric_limits<std::size_t>::max())
            index = skip;
        else
            index = next_whitespace;

        source.column = index - line_start_index;
        escaped = false;
    }

    return tokens;
}


auto
cchell::lexer::impl::verifier::operator()(
    const std::vector<token> &tokens) const -> std::optional<diagnostic>
{
    std::unordered_map<char, std::stack<source_location>> bracket {
        { '(', {} },
        { '{', {} },
        { '[', {} }
    };

    const token *quote { nullptr };

    for (const token &token : tokens)
    {
        if (token.type() == token_type::bracket)
        {
            char c { token.data()[0] };

            if (c == '(' || c == '{' || c == '[')
                bracket[c].push(token.source());
            else
            {
                char open { matching_open(c) };

                if (open == 0 || bracket[open].empty())
                    return diagnostic_builder { severity::error }
                        .domain("cchell::lexer")
                        .message("extra closing bracket '{}' found.", c)
                        .annotation("try removing the '{}'.", c)
                        .source(token.source())
                        .build();

                bracket[open].pop();
            }
            continue;
        }

        if (token.type() == token_type::quote)
        {
            if (quote == nullptr)
                quote = &token;
            else
                quote = nullptr;
        }
    }

    for (const auto &[open, stack] : bracket)
        if (!stack.empty())
            return diagnostic_builder { severity::error }
                .domain("cchell::lexer")
                .message("unclosed bracket '{}' found.", open)
                .annotation("consider adding a closing '{}'.", open)
                .source(stack.top())
                .build();

    if (quote != nullptr)
        return diagnostic_builder { severity::error }
            .domain("cchell::lexer")
            .message("unclosed quote {} found.", quote->data())
            .annotation("consider adding a closing {}.", quote->data())
            .source(quote->source())
            .build();

    return std::nullopt;
}
