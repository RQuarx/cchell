#include <algorithm>
#include <ranges>
#include <string_view>

#include "parser.hh"

#include "shared.cc" /* NOLINT */

using namespace cchell::parser;


namespace
{
    auto
    is_option(std::string_view data) -> bool
    {
        if (data.empty()) return false;
        if (!is_identifier_start(data.front(), true)) return false;
        if (std::ranges::count(data, '=') > 1) return false;

        return std::ranges::all_of(data | std::views::drop(1),
                                   [](char c) -> bool
                                   { return is_identifier_char(c, true); });
    }
}


auto
impl::option(const lexer::token &token, ast_node &parent) -> bool
{
    if (!is_option(token.data())) return false;

    std::size_t assign_index { token.data().find('=') };

    ast_node &root { parent.child.emplace_back(ast_node {}
                                                   .set_type(ast_type::option)
                                                   .set_source(token.source())
                                                   .set_parent(&parent)
                                                   .set_data(token.data())) };


    if (assign_index != std::string_view::npos)
    {
        cchell::source_location source { token.source() };
        source.column = assign_index + 1;

        split_key_value(token.data(), root,
                        { ast_type::identifier, ast_type::parameter },
                        { token.source(), source });
    }

    return true;
}

