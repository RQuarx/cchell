#include <interaction.hh>

#include <iostream>
#include <ranges>

#include <shared.hh>
#include <unistd.h>

using cchell::interaction::impl::ask;
using cchell::interaction::impl::question;


void
question::reset()
{
    message.clear();
    options.clear();
}


void
question::render()
{
    std::cout.flush();

    std::print(std::cerr, "{}ask{}: {} [{}/", style.tag_color, color::reset(),
               message, static_cast<char>(std::toupper(options[0])));

    for (auto i : std::views::iota(1UZ, options.length()))
        if (i == options.length() - 1)
            std::cerr << options[i];
        else
            std::cerr << options[i] << '/';
    std::cerr << "] ";
}


auto
ask::operator[](char option) -> ask &
{
    return m_question.options += option, *this;
}


auto
ask::operator[](const std::string &options) -> ask &
{
    return m_question.options = options, *this;
}


auto
ask::set_echo(bool echo) -> ask &
{
    return m_echo = echo, *this;
}


auto
ask::set_style(style style) -> ask &
{
    return m_question.style = style, *this;
}


auto
ask::get_last_error() const -> int
{
    return m_error;
}


auto
ask::mf_get_response() -> int
{
    char resp { '\0' };

    while (true)
    {
        if (auto n { ::read(STDIN_FILENO, &resp, 1) }; n < 0)
        {
            m_error = errno;
            return -1;
        }
        else if (n == 0) /* NOLINT */
        {
            m_error = EOF;
            return 0;
        }

        if (resp == '\n')
        {
            if (m_echo) std::putc(resp, stderr);
            return m_question.options.front();
        }

        if (m_question.options.contains(resp))
        {
            if (m_echo) std::cerr << resp << '\n';
            return resp;
        }
    }

    return -1; /* unknown error */
}
