#pragma once
#include <array>
#include <format>
#include <optional>
#include <string>

#include "color.hh"


namespace cchell
{
    namespace config
    {
        inline struct diagnostic_config
        {
            std::array<color, 3> tag_color {
                color { 220, 50,  47  }
                 .add_attribute(ansi::attribute::bold),
                color { 181, 137, 0   }
                 .add_attribute(ansi::attribute::bold),
                color { 38,  139, 210 }
                 .add_attribute(ansi::attribute::bold),
            };

            color line_color { 30, 31, 44 };
            color alt_line_color {
                color { 21, 22, 30 }
                 .add_attribute(ansi::attribute::dim)
            };

            color line_number_color {
                color { 193, 195, 211 }
                 .add_attribute(ansi::attribute::dim)
            };
            color separator_color { 100, 105, 140 };
            color underline_color {
                color { 220, 50, 47 }
                 .add_attribute(ansi::attribute::bold)
            };

            color error_code_color {
                color { 220, 50, 47 }
                 .add_attribute(ansi::attribute::bold)
            };
            color code_color { 255, 255, 255 };
            color domain_color {
                color { 116, 107, 215 }
                 .add_attribute(ansi::attribute::bold)
            };
            color source_color {
                color { 150, 150, 150 }
                 .add_attribute(ansi::attribute::dim)
            };
        } diagnostic;
    }


    class source_location
    {
    public:
        source_location(std::size_t line, std::size_t column)
            : m_line { line }, m_column { column }
        {
        }


        source_location(std::size_t single)
            : m_line { single }, m_column { single }
        {
        }


        source_location() = default;


        void
        operator()(std::size_t line, std::size_t column)
        {
            m_line   = line;
            m_column = column;
        }


        [[nodiscard]]
        auto
        line() const -> std::size_t
        {
            return m_line;
        }


        [[nodiscard]]
        auto
        column() const -> std::size_t
        {
            return m_column;
        }


        auto
        operator==(const source_location &other) const -> bool
        {
            return m_column == other.m_column && m_line == other.m_line;
        }

    private:
        std::size_t m_line;
        std::size_t m_column;
    };


    enum class message_level : std::uint8_t
    {
        error,
        warning,
        note
    };


    struct diagnostic
    {
        std::string_view domain;
        std::string_view raw;
        std::string_view file { "null" };
        source_location  source;
        std::size_t      length;

        std::string   message;
        std::string   annotation;
        message_level level;


        [[nodiscard]] auto to_string() -> std::string;
        [[nodiscard]] auto message_to_string() -> std::string;


        auto set_domain(std::string_view domain) -> diagnostic &;
        auto set_raw(std::string_view raw) -> diagnostic &;
        auto set_file(std::string_view file) -> diagnostic &;
        auto set_source(source_location source) -> diagnostic &;
        auto set_length(std::size_t length) -> diagnostic &;
        auto set_level(message_level level) -> diagnostic &;


        template <typename... T_Args>
        auto
        set_message(std::format_string<T_Args...> fmt, T_Args &&...args)
            -> diagnostic &
        {
            message = std::format(fmt, std::forward<T_Args>(args)...);
            return *this;
        }


        template <typename... T_Args>
        auto
        set_annotation(std::format_string<T_Args...> fmt, T_Args &&...args)
            -> diagnostic &
        {
            annotation = std::format(fmt, std::forward<T_Args>(args)...);
            return *this;
        }
    };


    template <typename T> struct verifier
    {
        [[nodiscard]]
        virtual auto operator()(T) const -> std::optional<diagnostic>
            = 0;
    };
}


namespace std
{
    template <> struct formatter<cchell::source_location>
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
            return format_to(ctx.out(), "{}:{}", source.line(),
                             source.column());
        }
    };
}
