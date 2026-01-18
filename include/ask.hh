#pragma once
#include <cctype>
#include <cstdio>
#include <format>
#include <print>
#include <ranges>
#include <vector>

#include <unistd.h>

#include "color.hh"


namespace cchell
{
    namespace config
    {
        inline struct ask_config
        {
            color tag_color {
                color { 38, 139, 210 }
                 .add_attribute(ansi::attribute::bold)
            };
        } ask;
    }


    namespace impl
    {
        class ask
        {
        public:
            auto
            operator[](char option) -> ask &
            {
                return m_options += option, *this;
            }


            template <typename... T_Args>
            auto
            operator()(std::format_string<T_Args...> fmt, T_Args &&...args)
                -> char
            {
                std::string message { std::format(
                    fmt, std::forward<T_Args>(args)...) };

                std::print("{}ask{}: {} [{}/", config::ask.tag_color,
                           color::reset(), message,
                           (char)std::toupper(m_options[0]));

                for (auto i : std::views::iota(1UZ, m_options.length()))
                    if (i == m_options.length() - 1)
                        std::print("{}", m_options[i]);
                    else
                        std::print("{}/", m_options[i]);

                std::print("] ");
                std::fflush(stdout);

                char response { 0 };
                while (true)
                {
                    ::read(STDIN_FILENO, &response, 1);

                    if (response == '\n')
                    {
                        if (m_echo) std::putchar('\n');
                        response = m_options.front();
                        break;
                    }

                    if (!m_options.contains(response)) continue;

                    if (m_echo) std::println("{}", response);
                    break;
                }

                m_options.clear();
                return response;
            }


            auto
            set_echo(bool echo) -> ask &
            {
                m_echo = echo;
                return *this;
            }


        private:
            std::string m_options;
            bool        m_echo { true };
        };
    }


    inline impl::ask ask;
}
