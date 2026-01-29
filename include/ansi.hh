#pragma once
#include <cstdint>
#include <format>


namespace cchell::ansi
{
    enum class command_kind : std::uint8_t
    {
        cursor_up,
        cursor_down,
        cursor_left,
        cursor_right,
        cursor_move, /* absolute */
        clear_screen,
        clear_line,
        save_cursor,
        restore_cursor,
        hide_cursor,
        show_cursor
    };


    struct command
    {
        command_kind  kind;
        std::uint16_t a { 0 };
        std::uint16_t b { 0 };


        static constexpr auto
        cursor_up(std::uint16_t n = 1) noexcept -> command
        {
            return { .kind = command_kind::cursor_up, .a = n };
        }


        static constexpr auto
        cursor_down(std::uint16_t n = 1) noexcept -> command
        {
            return { .kind = command_kind::cursor_down, .a = n };
        }


        static constexpr auto
        cursor_left(std::uint16_t n = 1) noexcept -> command
        {
            return { .kind = command_kind::cursor_left, .a = n };
        }


        static constexpr auto
        cursor_right(std::uint16_t n = 1) noexcept -> command
        {
            return { .kind = command_kind::cursor_right, .a = n };
        }


        static constexpr auto
        move_to(std::uint16_t row, std::uint16_t col) noexcept -> command
        {
            return { .kind = command_kind::cursor_move, .a = row, .b = col };
        }


        static constexpr auto
        clear_screen() noexcept -> command
        {
            return { .kind = command_kind::clear_screen };
        }


        static constexpr auto
        clear_line() noexcept -> command
        {
            return { .kind = command_kind::clear_line };
        }


        static constexpr auto
        save_cursor() noexcept -> command
        {
            return { .kind = command_kind::save_cursor };
        }


        static constexpr auto
        restore_cursor() noexcept -> command
        {
            return { .kind = command_kind::restore_cursor };
        }


        static constexpr auto
        hide_cursor() noexcept -> command
        {
            return { .kind = command_kind::hide_cursor };
        }


        static constexpr auto
        show_cursor() noexcept -> command
        {
            return { .kind = command_kind::show_cursor };
        }
    };
}


template <> struct std::formatter<cchell::ansi::command>
{
    static constexpr auto
    parse(std::format_parse_context &ctx) -> std::format_parse_context::iterator
    {
        return ctx.begin();
    }

    template <typename T_FormatContext>
    auto
    format(const cchell::ansi::command &cmd, T_FormatContext &ctx) const
        -> decltype(ctx.out())
    {
        using enum cchell::ansi::command_kind;
        auto out { ctx.out() };

        switch (cmd.kind)
        {
        case cursor_up:    return std::format_to(out, "\x1b[{}A", cmd.a);
        case cursor_down:  return std::format_to(out, "\x1b[{}B", cmd.a);
        case cursor_right: return std::format_to(out, "\x1b[{}C", cmd.a);
        case cursor_left:  return std::format_to(out, "\x1b[{}D", cmd.a);
        case cursor_move:
            return std::format_to(out, "\x1b[{};{}H", cmd.a, cmd.b);
        case clear_screen:   return std::format_to(out, "\x1b[2J");
        case clear_line:     return std::format_to(out, "\x1b[2K");
        case save_cursor:    return std::format_to(out, "\x1b[s");
        case restore_cursor: return std::format_to(out, "\x1b[u");
        case hide_cursor:    return std::format_to(out, "\x1b[?25l");
        case show_cursor:    return std::format_to(out, "\x1b[?25h");
        }
    }
};
