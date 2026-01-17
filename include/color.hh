#pragma once
#include <cstdint>
#include <format>
#include <utility>


namespace cchell
{
    namespace ansi
    {
        enum class attribute : std::uint8_t
        {
            none      = 0,
            bold      = 1 << 0,
            dim       = 1 << 1,
            underline = 1 << 2,
            blink     = 1 << 3,
            reverse   = 1 << 4,
        };


        enum class target : std::uint8_t
        {
            fg,
            bg
        };


        constexpr auto
        operator|(attribute a, attribute b) -> attribute
        {
            return static_cast<attribute>(static_cast<std::uint8_t>(a)
                                          | static_cast<std::uint8_t>(b));
        }

        constexpr auto
        operator&(attribute a, attribute b) -> attribute
        {
            return static_cast<attribute>(static_cast<std::uint8_t>(a)
                                          & static_cast<std::uint8_t>(b));
        }
    }


    struct alignas(alignof(std::uint32_t)) color
    {
        std::uint8_t r { 0 };
        std::uint8_t g { 0 };
        std::uint8_t b { 0 };
        std::uint8_t attribute { static_cast<std::uint8_t>(
            ansi::attribute::none) };


        constexpr color(std::uint8_t r, std::uint8_t g, std::uint8_t b)
            : r { r }, g { g }, b { b }
        {
        }

        constexpr auto
        set_attribute(ansi::attribute a) -> color &
        {
            attribute = static_cast<std::uint8_t>(a);
            return *this;
        }


        constexpr auto
        add_attribute(ansi::attribute a) -> color &
        {
            attribute |= static_cast<std::uint8_t>(a);
            return *this;
        }


        constexpr auto
        clear_attribuate(ansi::attribute a) -> color &
        {
            attribute &= ~static_cast<std::uint8_t>(a);
            return *this;
        }


        static constexpr auto
        reset() noexcept -> const char *
        {
            return "\x1b[0;0;0m";
        }


        static constexpr auto
        reset_fg() noexcept -> const char *
        {
            return "\x1b[39m";
        }


        static constexpr auto
        reset_bg() noexcept -> const char *
        {
            return "\x1b[49m";
        }


        static constexpr auto
        reset_attributes() noexcept -> const char *
        {
            return "\x1b[22;24;25;27m";
        }
    };
}


template <> struct std::formatter<cchell::color>
{
    enum class mode : std::uint8_t
    {
        ansi,
        hex
    } m_mode { mode::ansi };

    enum class ansi_target : std::uint8_t
    {
        fg,
        bg
    } m_target { ansi_target::fg };


    constexpr auto
    parse(std::format_parse_context &ctx)
    {
        const auto *it { ctx.begin() };
        const auto *end { ctx.end() };

        if (it == end || *it == '}') return it;

        if (std::string_view(it, end - it).starts_with("ansi"))
        {
            m_mode = mode::ansi;
            it    += 4;

            if (it != end && *it == '_')
            {
                it++;

                if (std::string_view(it, end - it).starts_with("fg"))
                {
                    m_target = ansi_target::fg;
                    it      += 2;
                }
                else if (std::string_view(it, end - it).starts_with("bg"))
                {
                    m_target = ansi_target::bg;
                    it      += 2;
                }
                else
                    throw std::format_error { "invalid ansi color target" };
            }
        }
        else if (std::string_view(it, end - it).starts_with("hex"))
        {
            m_mode = mode::hex;
            it    += 3;
        }
        else
            throw std::format_error { "invalid color format specifier" };

        if (it != end && *it != '}')
            throw std::format_error { "invalid color format" };

        return it;
    }


    template <typename FormatContext>
    auto
    format(const cchell::color &c, FormatContext &ctx) const
    {
        if (m_mode == mode::hex)
            return std::format_to(ctx.out(), "#{:02X}{:02X}{:02X}", c.r, c.g,
                                  c.b);

        bool first { true };
        auto out { ctx.out() };

        out = std::format_to(out, "\x1b[");

        auto emit { [&](int code) -> void
                    {
                        if (!first) out = std::format_to(out, ";");
                        out   = std::format_to(out, "{}", code);
                        first = false;
                    } };

        using namespace cchell;

#define IS_ATTR(attr) \
    (c.attribute & std::to_underlying(attr))

        if (IS_ATTR(ansi::attribute::bold)) emit(1);
        if (IS_ATTR(ansi::attribute::dim)) emit(2);
        if (IS_ATTR(ansi::attribute::underline)) emit(4);
        if (IS_ATTR(ansi::attribute::blink)) emit(5);
        if (IS_ATTR(ansi::attribute::reverse)) emit(7);
#undef IS_ATTR

        emit(m_target == ansi_target::fg ? 38 : 48);
        emit(2);
        emit(c.r);
        emit(c.g);
        emit(c.b);

        out = std::format_to(out, "m");
        return out;
    }
};
