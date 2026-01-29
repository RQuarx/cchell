#include <optional>
#include <tuple>

#include "terminal.hh"


namespace
{
    using namespace cchell::terminal;


    struct csi_state
    {
        bool active { false };
        int  p1 { 0 };
        int  p2 { 0 };
        bool semi { false };
    } g_state;


    auto
    decode_plain(char c) noexcept -> std::optional<key_event>
    {
        switch (c)
        {
        case '\r':
        case '\n': return key::enter;
        case '\t': return key::tab;
        case 0x7F: return key::backspace;
        default:   return std::nullopt;
        }
    }


    auto
    decode_csi_modifiers(int mod) noexcept
        -> std::tuple<bool, bool, bool> /* shift, alt, ctrl */
    {
        bool shift { false };
        bool alt { false };
        bool ctrl { false };

        switch (mod)
        {
        case 2:  shift = true; break;
        case 3:  alt = true; break;
        case 4:  shift = alt = true; break;
        case 5:  ctrl = true; break;
        case 6:  shift = ctrl = true; break;
        case 7:  alt = ctrl = true; break;
        case 8:  shift = alt = ctrl = true; break;
        default: return { shift, alt, ctrl };
        }

        return { shift, alt, ctrl };
    }


    auto
    decode_csi_final_byte(int p1, char final_byte, int mod) noexcept
        -> std::optional<key_event>
    {
        auto [shift, alt, ctrl] { decode_csi_modifiers(mod) };

        switch (final_byte)
        {
        case 'A': return key_event { key::arrow_up, shift, alt, ctrl };
        case 'B': return key_event { key::arrow_down, shift, alt, ctrl };
        case 'C': return key_event { key::arrow_right, shift, alt, ctrl };
        case 'D': return key_event { key::arrow_left, shift, alt, ctrl };
        case 'H': return key_event { key::home, shift, alt, ctrl };
        case 'F': return key_event { key::end, shift, alt, ctrl };
        case '~':
            switch (p1)
            {
            case 1:  return key_event { key::home, shift, alt, ctrl };
            case 2:  return key_event { key::insert, shift, alt, ctrl };
            case 3:  return key_event { key::del, shift, alt, ctrl };
            case 4:  return key_event { key::end, shift, alt, ctrl };
            case 5:  return key_event { key::page_up, shift, alt, ctrl };
            case 6:  return key_event { key::page_down, shift, alt, ctrl };
            default: break;
            }
        default: return std::nullopt;
        }
    }


    auto
    decode_csi_char(char ch) noexcept -> decode_result
    {
        using enum decode_status;

        if (!g_state.active)
        {
            if (ch == '[')
            {
                g_state        = {};
                g_state.active = true;
                return { pending, std::nullopt };
            }

            return { none, std::nullopt };
        }

        if (ch >= '0' && ch <= '9')
        {
            int &target { g_state.semi ? g_state.p2 : g_state.p1 };
            target = (target * 10) + (ch - '0');

            return { pending, std::nullopt };
        }

        if (ch == ';')
        {
            g_state.semi = true;
            return { pending, std::nullopt };
        }

        /* final byte */
        int  mod { g_state.semi ? g_state.p2 : 1 };
        auto event { decode_csi_final_byte(g_state.p1, ch, mod) };

        g_state = {}; /* reset */

        if (!event) return { none, std::nullopt };

        return { value, event };
    }
}


auto
cchell::terminal::decode(char ch) noexcept -> decode_result
{
    using enum decode_status;

    static bool esc_seen { false };

    if (!esc_seen)
    {
        if (ch == 0x1B) /* ESC */
        {
            esc_seen = true;
            return { pending, std::nullopt };
        }

        if (auto event { decode_plain(ch) }) return { value, event };

        return { none, std::nullopt };
    }

    /* ESC already seen */
    esc_seen = false;

    /* Alt + key */
    if (ch != '[')
    {
        return {
            value, key_event { key::unknown, false, true }
        };
    }

    /* CSI */
    return decode_csi_char(ch);
}
