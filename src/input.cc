#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstring>

#include <termios.h>
#include <unistd.h>

#include "input.hh"
#include "shared.hh"

using cchell::input;


auto
input::create() noexcept -> std::expected<input, std::string>
{
    input inp;

    if (shared::tty_status.stdin())
    {
        if (tcgetattr(STDIN_FILENO, &inp.m_old_term) < 0)
            return std::unexpected { std::strerror(errno) };

        termios newt { inp.m_old_term };
        newt.c_lflag    &= ~(ICANON | ECHO);
        newt.c_cc[VMIN]  = 1;
        newt.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) < 0)
            return std::unexpected { std::strerror(errno) };

        std::signal(SIGINT, input::sigint_handler);
    }
    else
        cchell::input::m_instance = nullptr;

    return inp;
}


input::input() { input::m_instance = this; }


input::input(input &&other) noexcept
    : m_old_term { other.m_old_term },
      m_sigint_triggered { other.m_sigint_triggered.load() }
{
    if (m_instance == &other) m_instance = this;
    other.m_sigint_triggered.store(false);
}


auto
input::operator=(input &&other) noexcept -> input &
{
    if (this == &other) return *this;

    m_old_term = other.m_old_term;
    m_sigint_triggered.store(other.m_sigint_triggered.load());

    if (m_instance == &other) m_instance = this;

    other.m_sigint_triggered.store(false);

    return *this;
}


void
input::sigint_handler(int sig)
{
    if (sig == SIGINT && m_instance != nullptr)
    {
        m_instance->m_sigint_triggered.store(true, std::memory_order_relaxed);
        std::signal(SIGINT, input::sigint_handler);
    }
}


auto
input::read(std::string &text) noexcept -> int
{
    text.clear();

    if (shared::tty_status.stdin())
        return read_stdin(text);
    return 0;
}


auto
input::read_stdin(std::string &text) noexcept -> int
{
    bool reading { true };
    bool escaped { false };
    char ch;

    while (reading)
    {
        if (auto n { ::read(STDIN_FILENO, &ch, 1) }; n < 0)
            return errno;
        else if (n == 0) /* NOLINT: Do not use 'else' after 'return' */
            return EOF;

        

    }
}