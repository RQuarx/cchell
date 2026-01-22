#include <algorithm>
#include <ranges>
#include <string_view>

#include "lexer.hh"
#include "parser.hh"
#include "parser/impl.hh"

#include "shared.cc" /* NOLINT */

using namespace cchell::parser;


namespace { std::uint8_t inside_string { 0 }; }


auto
impl::string(const lexer::token &token, ast_node &parent) -> bool
{
    if (inside_string == 0 && token.type() != lexer::token_type::quote)
        return false;

    if (inside_string == 0)
    {
        inside_string++;
        return true;
    }

    if (inside_string == 2)
    {
        inside_string = 0;
        return true;
    }

    parent.child.emplace_back(ast_node {}
                                  .set_type(ast_type::option)
                                  .set_source(token.source())
                                  .set_parent(&parent)
                                  .set_data(token.data()));
    inside_string++;

    return true;
}
