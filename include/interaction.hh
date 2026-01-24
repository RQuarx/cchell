#pragma once
#include <format>

#include "color.hh"


namespace cchell::interaction
{
    struct theme
    {
        color tag_color {
            color { 38, 139, 210 }
             .add_attribute(ansi::attribute::bold)
        };
    };

    inline constexpr theme default_theme {};


    namespace impl
    {
        struct question
        {
            std::string message;
            std::string options;
            theme       theme;

            void reset();
            void render();
        };


        class ask
        {
        public:
            auto operator[](char option) -> ask &;
            auto operator[](const std::string &options) -> ask &;

            auto set_echo(bool echo) -> ask &;
            auto set_theme(theme theme = default_theme) -> ask &;

            [[nodiscard]] auto get_last_error() const -> int;


            template <typename... T_Args>
            auto
            operator()(std::format_string<T_Args...> fmt, T_Args &&...args)
                -> char
            {
                m_question.message
                    = std::format(fmt, std::forward<T_Args>(args)...);

                m_question.render();

                int res { mf_get_response() };
                if (res <= 0) return '\0';

                m_question.reset();
                return static_cast<char>(res);
            }

        private:
            question m_question;
            bool     m_echo { true };
            int      m_error;


            /**
             * wait and returns a response from the user.
             *
             * the function can returns a value > 0 for a valid value,
             * 0 on EOF, or -1 on errors.
             */
            auto mf_get_response() -> int;
        };
    }


    inline impl::ask ask;
}
