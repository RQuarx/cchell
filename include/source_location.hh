#pragma once
#include <format>


namespace cchell
{
    struct source_location
    {
        std::uint32_t line { 0 };
        std::uint32_t column { 0 };

        auto
        operator==(const source_location &other) const -> bool
        {
            return line == other.line && column == other.column;
        }
    };
}


template <> struct std::formatter<cchell::source_location>
{
    static constexpr auto
    parse(format_parse_context &ctx)
    {
        return ctx.begin();
    }


    template <class T_FmtContext>
    auto
    format(cchell::source_location source, T_FmtContext &ctx) const
    {
        return format_to(ctx.out(), "{}:{}", source.line, source.column);
    }
};