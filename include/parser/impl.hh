#pragma once

namespace cchell::lexer { class token; }


namespace cchell::parser
{
    struct ast_node;

    namespace impl
    {
        auto assignment(const lexer::token &token, ast_node &parent) -> bool;
        auto command(const lexer::token &token, ast_node &parent) -> bool;
        auto option(const lexer::token &token, ast_node &parent) -> bool;
    }

}
