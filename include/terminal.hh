#pragma once
#include <cstdint>
#include <optional>


namespace cchell::terminal
{
    enum class key : std::uint8_t
    {
        unknown,

        /* ASCII */
        enter,
        escape,
        backspace,
        tab,

        /* arrows */
        arrow_up,
        arrow_down,
        arrow_left,
        arrow_right,

        /* navigation */
        home,
        end,
        page_up,
        page_down,
        insert,
        del,

        /* function keys */
        f1,
        f2,
        f3,
        f4,
        f5,
        f6,
        f7,
        f8,
        f9,
        f10,
        f11,
        f12
    };


    struct key_event
    {
        key  code;
        bool shift { false };
        bool alt { false };
        bool ctrl { false };


        key_event(key  code,
                  bool alt   = false,
                  bool ctrl  = false,
                  bool shift = false)
            : code { code }, shift { shift }, alt { alt }, ctrl { ctrl }
        {
        }
    };


    enum class decode_status : std::uint8_t
    {
        value,
        pending,
        none,
    };


    struct decode_result
    {
        decode_status            status;
        std::optional<key_event> event;

        decode_result(decode_status            status,
                      std::optional<key_event> event = std::nullopt)
            : status { status }, event { event }
        {
        }
    };


    [[nodiscard]]
    auto decode(char ch) noexcept -> decode_result;
}
