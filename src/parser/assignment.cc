#include <algorithm>
#include <ranges>

#include "parser.hh"
#include "parser/impl.hh"

#include "shared.cc" /* NOLINT */

using namespace cchell::parser;


namespace
{
    auto
    is_assignment(std::string_view data) -> bool
    {
        if (data.empty()) return false;
        if (!is_identifier_start(data.front())) return false;
        if (std::ranges::count(data, '=') > 1) return false;

        return std::ranges::all_of(data | std::views::drop(1),
                                   [](char c) -> bool
                                   { return is_identifier_char(c, false); });
    }
}


auto
impl::assignment(const lexer::token &token, ast_node &parent) -> bool
{
    if (!is_assignment(token.data())) return false;

    std::size_t assign_index { token.data().find('=') };
    if (assign_index == std::string_view::npos) return false;

    auto &root { parent->emplace_back(ast_node {}
                                          .set_type(ast_type::assignment)
                                          .set_source(token.source())
                                          .set_parent(&parent)) };

    cchell::source_location source { token.source() };
    source(source.line(), assign_index + 1);

    split_key_value(token.data(), root,
                    { ast_type::identifier, ast_type::literal },
                    { token.source(), source });

    return true;
}
