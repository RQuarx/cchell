#include <cctype>
#include <string_view>

#include "parser.hh"


namespace
{
    using namespace cchell::parser;

    template <typename T> using tpair = std::pair<T, T>;


    [[maybe_unused]]
    constexpr std::string_view RESERVED_CHAR { "\"'`(){}[]<>/$|" };


    [[maybe_unused]]
    constexpr auto
    is_identifier_start(char c, bool is_option = true) -> bool
    {
        return std::isalpha(c) != 0 || c == '_' || (is_option && c == '-');
    }


    [[maybe_unused]]
    constexpr auto
    is_identifier_char(char c, bool is_option = true) -> bool
    {
        return std::isalnum(c) != 0 || c == '_' || c == '=' || c == '\\'
            || (is_option && (c == '-'));
    }


    [[maybe_unused]]
    auto
    split_key_value(std::string_view               data,
                    ast_node                      &parent,
                    tpair<ast_type>                type,
                    tpair<cchell::source_location> source) -> bool
    {
        auto &[key_type, value_type] { type };
        auto &[key_source, value_source] { source };

        std::string_view key { data.substr(0, value_source.column - 1) };
        std::string_view value { data.substr(value_source.column) };

        parent.child.emplace_back(ast_node {}
                                      .set_type(key_type)
                                      .set_source(key_source)
                                      .set_parent(&parent)
                                      .set_data(key));

        parent.child.emplace_back(ast_node {}
                                      .set_type(value_type)
                                      .set_source(value_source)
                                      .set_parent(&parent)
                                      .set_data(value));
        return true;
    }
}
