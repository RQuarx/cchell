#pragma once
#include <optional>

namespace cchell::lexer { class token; }
namespace cchell::diagnostics { struct diagnostic; }


namespace cchell::parser
{
    struct ast_node;

    namespace impl
    {
        auto assignment(const lexer::token &token, ast_node &parent) -> bool;
        auto command(const lexer::token &token, ast_node &parent) -> bool;
        auto option(const lexer::token &token, ast_node &parent) -> bool;
        auto string(const lexer::token &token, ast_node &parent) -> bool;

        using diagnostics::diagnostic;

        auto verify_command(ast_node &node) -> std::optional<diagnostic>;
    }
}
