#pragma once
#include <format>
#include <iomanip>
#include <optional>
#include <string_view>
#include <vector>

#include "diagnostic.hh"


namespace cchell::lexer
{
    enum class token_type : std::uint8_t
    {
        word,
        parenthesis,
        quote,
        path,
        pipe,
        dollar,
        none
    };


    class token
    {
    public:
        token(token_type type, std::string_view data, source_location source)
            : m_type { type }, m_data { data }, m_source { source }
        {
        }


        [[nodiscard]]
        auto
        type() const -> token_type
        {
            return m_type;
        }


        [[nodiscard]]
        auto
        data() const -> std::string_view
        {
            return m_data;
        }


        [[nodiscard]]
        auto
        source() const -> source_location
        {
            return m_source;
        }

    private:
        token_type       m_type;
        std::string_view m_data;

        source_location m_source;
    };


    [[nodiscard]]
    auto lex(std::string_view string) -> std::vector<token>;


    [[nodiscard]]
    auto verify(const std::vector<token> &tokens) -> std::optional<diagnostic>;
}


namespace std
{
    template <> struct formatter<cchell::lexer::token_type>
    {
        static constexpr auto
        parse(format_parse_context &ctx)
        {
            return ctx.begin();
        }


        template <class T_FmtContext>
        auto
        format(cchell::lexer::token_type type, T_FmtContext &ctx) const
            -> T_FmtContext::iterator
        {
            using cchell::lexer::token_type;

            string_view name;

            switch (type)
            {
            case token_type::word: name = "token_type::word"; break;
            case token_type::parenthesis:
                name = "token_type::parenthesis";
                break;
            case token_type::quote:  name = "token_type::quote"; break;
            case token_type::path:   name = "token_type::path"; break;
            case token_type::pipe:   name = "token_type::pipe"; break;
            case token_type::dollar: name = "token_type::dollar"; break;
            case token_type::none:   name = "token_type::none"; break;
            }

            return format_to(ctx.out(), "{}", name);
        }
    };


    template <> struct formatter<cchell::lexer::token>
    {
        static constexpr auto
        parse(format_parse_context &ctx)
        {
            return ctx.begin();
        }


        template <class T_FmtContext>
        auto
        format(const cchell::lexer::token &token, T_FmtContext &ctx) const
            -> T_FmtContext::iterator
        {
            string data { (ostringstream {} << quoted(token.data())).str() };

            return format_to(ctx.out(),
                             R"({{
  "source": [ {} ],
  "data":   {},
  "type":   {}
}})",
                             token.source(), data, token.type());
        }
    };
}
