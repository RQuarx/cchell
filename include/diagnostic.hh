#pragma once
#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <string>

#include "color.hh"


namespace cchell
{
    struct source_location
    {
        std::size_t line { 0 };
        std::size_t column { 0 };

        auto
        operator==(const source_location &other) const -> bool
        {
            return line == other.line && column == other.column;
        }
    };
}


namespace cchell::diagnostics
{
    struct theme
    {
        std::array<color, 3> tag_color {
            color { 220, 50,  47  }
             .add_attribute(
                ansi::attribute::bold), /* error */
            color { 181, 137, 0   }
             .add_attribute(
                ansi::attribute::bold), /* warning */
            color { 38,  139, 210 }
             .add_attribute(
                ansi::attribute::bold)  /* note */
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


        std::size_t extra_shown_line { 2 };
        std::size_t right_padding { 5 };
    };


    inline constexpr theme default_theme {};


    enum class severity : std::uint8_t
    {
        error,
        warning,
        note
    };


    struct diagnostic
    {
        friend class diagnostic_builder;

        severity         level;
        std::string      message;
        std::string      annotation;
        std::string_view domain;

        source_location source;
        std::size_t     length { 1 };


        [[nodiscard]]
        auto render(std::string_view raw_string,
                    std::string_view input_file,
                    const theme     &theme = default_theme) -> std::string;

    private:
        std::size_t m_padding;
        std::size_t m_line_number_width;
        std::string m_rendered;


        [[nodiscard]]
        auto format_line(std::size_t      line_num,
                         std::string_view line,
                         std::size_t      line_len,
                         const theme     &theme) const -> std::string;

        [[nodiscard]]
        auto create_colorless_source(std::string_view input_file) const
            -> std::string;


        void render_header(const theme &theme);

        void render_source(std::string_view colorless_source,
                           const theme     &theme);

        void render_line(std::size_t      line_num,
                         std::string_view line,
                         bool             error_line,
                         const theme     &theme);

        void render_annotation(const theme &theme);


        void render_colored(std::string_view raw_string,
                            std::string_view input_file,
                            const theme     &theme);
        void render_colorless(std::string_view input_file);
    };


    class diagnostic_builder
    {
    public:
        explicit diagnostic_builder(severity lvl);


        auto domain(std::string_view domain) -> diagnostic_builder &&;
        auto source(source_location source) -> diagnostic_builder &&;
        auto length(std::size_t length) -> diagnostic_builder &&;

        [[nodiscard]] auto build() && -> diagnostic;


        template <typename... T_Args>
        auto
        message(std::format_string<T_Args...> fmt, T_Args &&...args)
            -> diagnostic_builder &&
        {
            m_diag.message = std::format(fmt, std::forward<T_Args>(args)...);
            return std::move(*this);
        }


        template <typename... T_Args>
        auto
        annotation(std::format_string<T_Args...> fmt, T_Args &&...args)
            -> diagnostic_builder &&
        {
            m_diag.annotation = std::format(fmt, std::forward<T_Args>(args)...);
            return std::move(*this);
        }

    private:
        diagnostic m_diag;
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
            return format_to(ctx.out(), "{}:{}", source.line, source.column);
        }
    };
}
