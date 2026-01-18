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
        set_type(ast_type type) -> ast_node &
        {
            this->type = type;
            return *this;
        }


        auto
        set_parent(ast_node *parent) -> ast_node &
        {
            this->parent = parent;
            return *this;
        }


        auto
        set_data(std::string_view data) -> ast_node &
        {
            this->data = data;
            return *this;
        }


        auto
        set_source(source_location source) -> ast_node &
        {
            this->source = source;
            return *this;
        }


        auto
        operator->() -> std::list<ast_node> *
        {
            return &child;
        }


        auto
        operator->() const -> const std::list<ast_node> *
        {
            return &child;
        }
    };


    [[nodiscard]]
    auto parse(const std::vector<lexer::token> &tokens)
        -> std::unique_ptr<ast_node>;
}


namespace std
{
    template <> struct formatter<cchell::parser::ast_type>
    {
        static constexpr auto
        parse(format_parse_context &ctx)
        {
            return ctx.begin();
        }

        template <typename T_FormatContext>
        auto
        format(cchell::parser::ast_type type, T_FormatContext &ctx) const
        {
            using enum cchell::parser::ast_type;

            string_view name;
            switch (type)
            {
            case statement:  name = "statement"; break;
            case command:    name = "command"; break;
            case option:     name = "option"; break;
            case parameter:  name = "parameter"; break;
            case assignment: name = "assignment"; break;
            case identifier: name = "identifier"; break;
            case literal:    name = "literal"; break;
            default:         name = "<unknown>"; break;
            }

            return format_to(ctx.out(), "{}", name);
        }
    };


    template <> struct formatter<cchell::parser::ast_node>
    {
        static constexpr auto
        parse(format_parse_context &ctx)
        {
            return ctx.begin();
        }


        template <typename T_FormatContext>
        auto
        format(const cchell::parser::ast_node &node, T_FormatContext &ctx) const
        {
            mf_format_node(ctx, node, "", true);
            return ctx.out();
        }


    private:
        template <typename T_FormatContext>
        static void
        mf_format_node(T_FormatContext                &ctx,
                       const cchell::parser::ast_node &node,
                       const string                   &prefix,
                       bool                            is_last)
        {
            auto out { ctx.out() };

            if (!prefix.empty())
                out = format_to(out, "{}{}", prefix, is_last ? "└── " : "├── ");

            out = format_to(out, "{}", node.type);

            if (!node.data.empty()) out = format_to(out, " \"{}\"", node.data);

            out = format_to(out, "\n");

            std::string child_prefix { prefix };
            child_prefix += (is_last ? "    " : "│   ");

            auto it { node.child.begin() };
            auto end { node.child.end() };

            for (; it != end; it++)
            {
                bool last { next(it) == end };
                mf_format_node(ctx, *it, child_prefix, last);
            }
        }
    };

}
