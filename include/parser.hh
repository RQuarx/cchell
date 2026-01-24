#pragma once
#include <cstdint>
#include <list>
#include <memory>
#include <string_view>
#include <vector>

#include "diagnostic.hh"
#include "lexer.hh"

namespace cchell::lexer { class token; }


namespace cchell::parser
{
    enum class ast_type : std::uint8_t
    {
        statement, /* root */
        command,
        option,
        parameter,
        assignment,
        identifier,
        literal,
    };


    struct ast_node
    {
        ast_type            type;
        ast_node           *parent;
        std::list<ast_node> child;

        std::string_view data;
        source_location  source;


        auto
        set_type(ast_type type) noexcept -> ast_node &
        {
            this->type = type;
            return *this;
        }


        auto
        set_parent(ast_node *parent) noexcept -> ast_node &
        {
            this->parent = parent;
            return *this;
        }


        auto
        set_data(std::string_view data) noexcept -> ast_node &
        {
            this->data = data;
            return *this;
        }


        auto
        set_source(source_location source) noexcept -> ast_node &
        {
            this->source = source;
            return *this;
        }
    };


    [[nodiscard]]
    auto parse(const std::vector<lexer::token> &tokens)
        -> std::unique_ptr<ast_node>;


    namespace impl
    {
        auto assignment(const lexer::token &token, ast_node &parent) -> bool;
        auto command(const lexer::token &token, ast_node &parent) -> bool;
        auto option(const lexer::token &token, ast_node &parent) -> bool;
        auto string(const lexer::token &token, ast_node &parent) -> bool;

        using namespace diagnostics;

        struct verifier : diagnostics::verifier<ast_node &>
        {
            [[nodiscard]]
            auto operator()(ast_node &nodes) const
                -> std::optional<diagnostic> override;
        };

        auto verify_command(ast_node &node) -> std::optional<diagnostic>;
    }

    inline constexpr impl::verifier verify;
}
