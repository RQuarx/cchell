#include <algorithm>
#include <ranges>

#include "parser.hh"
#include "parser/impl.hh"

#include "shared.cc" /* NOLINT */

using namespace cchell::parser;


namespace
{
    auto
    is_command(std::string_view data) -> bool
    {
        if (data.empty()) return false;
        bool is_local { data.starts_with("./") };

        return std::ranges::all_of(std::views::iota(0UZ, data.length()),
                                   [data, is_local](std::size_t i) -> bool
                                   {
                                       if (std::isalnum(data[i]) != 0)
                                           return true;
                                       if (is_local && data[i] == '/')
                                           return true;
                                       if (i > 0
                                           && RESERVED_CHAR.contains(data[i])
                                           && data[i - 1] == '\\')
                                           return true;

                                       return false;
                                   });
    }
}


auto
impl::command(const lexer::token &token, ast_node &parent) -> bool
{
    if (!is_command(token.data())) return false;

    parent->emplace_back(ast_node {}
                             .set_type(ast_type::command)
                             .set_source(token.source())
                             .set_parent(&parent)
                             .set_data(token.data()));

    return true;
}
