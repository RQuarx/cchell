#pragma once
#include <cstdint>
#include <list>
#include <string>


namespace cchell::parser
{
    enum class ast_type : std::uint8_t
    {
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
        std::list<ast_node> child;
        ast_node           *parent;

        std::string data;

        std::size_t index;
    };
}
