#include <ranges>
#include <stack>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "diagnostic.hh"
#include "lexer.hh"


namespace
{
    [[nodiscard]]
    auto
    is_punct(char c) -> bool
    {
        return std::string_view { "`!$%^&*(){}[]|;:\"'<>,?" }.find(c)
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


    [[nodiscard]]
    auto
    get_punct_token_type(char c) -> cchell::lexer::token_type
    {
        using cchell::lexer::token_type;

        switch (c)
        {
        case '\'': [[fallthrough]];
        case '"':  [[fallthrough]];
        case '`':  return token_type::quote;

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


    void
    get_tokens_from_word(std::string_view                   word,
                         std::size_t                        index,
                         std::size_t                        line_start_index,
                         cchell::source_location            source,
                         std::vector<cchell::lexer::token> &out)
    {
        using namespace cchell::lexer;

        std::size_t start { 0 };

        for (std::size_t i { 0 }; i < word.length(); i++)
        {
            if (!is_punct(word[i])) continue;

            if (i > start)
            {
                source(source.line(), (index + start) - line_start_index);
                out.emplace_back(token_type::word,
                                 word.substr(start, i - start), source);
            }

            source(source.line(), index + i);
            out.emplace_back(get_punct_token_type(word[i]), word.substr(i, 1),
                             source);

            start = i + 1;
        }

        if (start < word.length())
        {
            source(source.line(), (index + start) - line_start_index);
            out.emplace_back(token_type::word, word.substr(start), source);
        }
    }

}


auto
cchell::lexer::lex(std::string_view string) -> std::vector<token>
{
    std::vector<token> tokens;
    tokens.reserve(string.size() / 2);

    std::size_t     index { 0 };
    std::size_t     line_start_index { 0 };
    source_location source { 0, 0 };

    while (index < string.length())
    {
        char c { string[index] };

        if (c == '\n')
        {
            source(source.line() + 1, 0);
            line_start_index = ++index;
            continue;
        }

        if (std::isspace(c) != 0)
        {
            index++;
            source(source.line(), index - line_start_index);
            continue;
        }

        std::size_t next_whitespace { index + 1 };
        while (next_whitespace < string.length()
               && std::isspace(string[next_whitespace]) == 0)
            next_whitespace++;

        std::string_view word { string.data() + index,
                                next_whitespace - index };

        get_tokens_from_word(word, index, line_start_index, source, tokens);

        index = next_whitespace;
        source(source.line(), index - line_start_index);
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

    std::optional<char> active_quote;
    source_location     quote_location {};

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
                    return diagnostic {}
                        .set_domain("cchell::lexer")
                        .set_message("extra closing bracket '{}' found.", c)
                        .set_annotation("try removing the '{}'.", c)
                        .set_level(message_level::error)
                        .set_source(token.source())
                        .set_length(1);

                bracket[open].pop();
            }
            continue;
        }

        if (token.type() == token_type::quote)
        {
            char q { token.data()[0] };

            if (!active_quote)
            {
                active_quote   = q;
                quote_location = token.source();
            }
            else if (*active_quote == q)
                active_quote.reset();
        }
    }

    for (const auto &[open, stack] : bracket)
        if (!stack.empty())
            return diagnostic {}
                .set_domain("cchell::lexer")
                .set_message("unclosed bracket '{}' found.", open)
                .set_annotation("consider adding a closing '{}'.", open)
                .set_level(message_level::error)
                .set_source(stack.top())
                .set_length(1);

    if (active_quote)
        return diagnostic {}
            .set_domain("cchell::lexer")
            .set_message("unclosed quote {} found.", *active_quote)
            .set_annotation("consider adding a closing {}.", *active_quote)
            .set_level(message_level::error)
            .set_source(quote_location)
            .set_length(1);

    return std::nullopt;
}
