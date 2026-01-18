#include <algorithm>
#include <cctype>
#include <ranges>

#include "diagnostic.hh"
#include "lexer.hh"
#include "parser.hh"


namespace
{
    constexpr std::string_view RESERVED_CHAR { "\"'`(){}[]<>/$|" };


    constexpr auto
    is_identifier_start(char c, bool is_option = true) -> bool
    {
        return std::isalpha(c) != 0 || c == '_' || (is_option && c == '-');
    }


    constexpr auto
    is_identifier_char(char c, bool is_option = true) -> bool
    {
        return std::isalnum(c) != 0 || c == '_' || (is_option && (c == '-' || c == '='));
    }


    auto
    is_assignment_identifier(std::string_view data) -> bool
    {
        if (data.empty()) return false;
        if (!is_identifier_start(data.front())) return false;

        return std::ranges::all_of(data | std::views::drop(1),
                                   [](char c) -> bool
                                   { return is_identifier_char(c, false); });
    }


    auto
    handle_assignment(const cchell::lexer::token &token,
                      cchell::parser::ast_node   &parent) -> bool
    {
        std::size_t assign_index { token.data().find('=') };
        if (assign_index == std::string_view::npos) return false;

        std::string_view identifier { token.data().substr(0, assign_index) };
        if (!is_assignment_identifier(identifier)) return false;

        using namespace cchell::parser;

        auto &root { parent->emplace_back(ast_node {}
                                              .set_type(ast_type::assignment)
                                              .set_source(token.source())
                                              .set_parent(&parent)) };
        auto &lhs { root->emplace_back(ast_node {}
                                           .set_type(ast_type::identifier)
                                           .set_source(token.source())
                                           .set_parent(&root)
                                           .set_data(identifier)) };

        std::string_view        value { token.data().substr(assign_index + 1) };
        cchell::source_location source { token.source() };
        source(source.line(), source.column() + lhs.data.length() + 1);

        root->emplace_back(ast_node {}
                               .set_type(ast_type::literal)
                               .set_source(source)
                               .set_parent(&root)
                               .set_data(value));
        return true;
    }


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


    auto
    handle_command(const cchell::lexer::token &token,
                   cchell::parser::ast_node   &parent) -> bool
    {
        if (!is_command(token.data())) return false;

        using namespace cchell::parser;
        parent->emplace_back(ast_node {}
                                 .set_type(ast_type::command)
                                 .set_source(token.source())
                                 .set_parent(&parent)
                                 .set_data(token.data()));

        return true;
    }


    auto
    is_option(std::string_view data) -> bool
    {
        if (!is_identifier_start(data.front(), true)) return false;

        return std::ranges::all_of(data | std::views::drop(1),
                                   [](char c) -> bool
                                   { return is_identifier_char(c, true); });
    }


    auto
    handle_option(const cchell::lexer::token &token,
                  cchell::parser::ast_node   &parent) -> bool
    {
        if (!is_option(token.data())) return false;

        using namespace cchell::parser;

        std::size_t assign_index { token.data().find('=') };
        if (assign_index == std::string_view::npos)
        {
            parent->emplace_back(ast_node {}
                                     .set_type(ast_type::option)
                                     .set_source(token.source())
                                     .set_parent(&parent)
                                     .set_data(token.data()));
            return true;
        }

        std::string_view identifier { token.data().substr(0, assign_index) };

        auto &root { parent->emplace_back(ast_node {}
                                              .set_type(ast_type::option)
                                              .set_source(token.source())
                                              .set_parent(&parent)) };
        auto &lhs { root->emplace_back(ast_node {}
                                           .set_type(ast_type::identifier)
                                           .set_source(token.source())
                                           .set_parent(&root)
                                           .set_data(identifier)) };

        std::string_view        value { token.data().substr(assign_index + 1) };
        cchell::source_location source { token.source() };
        source(source.line(), source.column() + lhs.data.length() + 1);

        root->emplace_back(ast_node {}
                               .set_type(ast_type::parameter)
                               .set_source(source)
                               .set_parent(&root)
                               .set_data(value));
        return true;
    }
}


auto
cchell::parser::parse(const std::vector<lexer::token> &tokens)
    -> std::unique_ptr<ast_node>
{
    if (auto diag { lexer::verify(tokens) }) throw *diag;

    auto root { std::make_unique<ast_node>(
        ast_node {}.set_type(ast_type::statement)) };

    bool found_command { false };

    for (const lexer::token &token : tokens)
    {
        if (!found_command)
        {
            if (handle_assignment(token, *root)) continue;
            if (handle_command(token, *root))
            {
                found_command = true;
                continue;
            }

            continue;
        }

        if (handle_option(token, *root)) continue;
    }

    return root;
}
