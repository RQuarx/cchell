#pragma once
#if PROJECT_IS_RELEASE
#include <format>
#include <iomanip>
#include <sstream>

#include "parser.hh"


namespace std
{
    template <> struct formatter<cchell::lexer::token_type>
    {
        static constexpr auto
        parse(format_parse_context &ctx)
        {
            return ctx.begin();
        }


        template <class T_FmtContext>
        auto
        format(cchell::lexer::token_type type, T_FmtContext &ctx) const
            -> T_FmtContext::iterator
        {
            using cchell::lexer::token_type;

            string_view name;

            switch (type)
            {
            case token_type::word:    name = "token_type::word"; break;
            case token_type::bracket: name = "token_type::bracket"; break;
            case token_type::quote:   name = "token_type::quote"; break;
            case token_type::pipe:    name = "token_type::pipe"; break;
            case token_type::dollar:  name = "token_type::dollar"; break;
            case token_type::none:    name = "token_type::none"; break;
            }

            return format_to(ctx.out(), "{}", name);
        }
    };


    template <> struct formatter<cchell::lexer::token>
    {
        static constexpr auto
        parse(format_parse_context &ctx)
        {
            return ctx.begin();
        }


        template <class T_FmtContext>
        auto
        format(const cchell::lexer::token &token, T_FmtContext &ctx) const
            -> T_FmtContext::iterator
        {
            string data { (ostringstream {} << quoted(token.data())).str() };

            /* clang-format off */
            return format_to(ctx.out(),
R"({{
  "source": [ {} ],
  "data":   {},
  "type":   {}
}})", token.source(), data, token.type());
            /* clang-format on */
        }
    };


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

#endif
