#pragma once
#include <optional>
#include <string_view>
#include <vector>

#include "diagnostic.hh"


namespace cchell::lexer
{
    enum class token_type : std::uint8_t
    {
        /* a generic "word" token (command, options, envars, etc) */
        word,

        /* a token that represents an open or
           closing bracket ('{}', '()', '[]') */
        bracket,

        /* a token that represents a quote type (', ", `) */
        quote,

        /* a token that represents the pipe (|) operator */
        pipe,

        /* a token that represents a dollar-sign ($) */
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
        source_location  m_source;
    };


    [[nodiscard]]
    auto lex(std::string_view string) -> std::vector<token>;


    namespace impl
    {
        using namespace diagnostics;

        struct verifier : diagnostics::verifier<const std::vector<token> &>
        {
            [[nodiscard]]
            auto operator()(const std::vector<token> &tokens) const
                -> std::optional<diagnostic> override;
        };
    }


    inline constexpr impl::verifier verify;
}
