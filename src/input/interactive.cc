#include <atomic>
#include <csignal>
#include <cstring>
#include <stdexcept>
#include <utility>

#include <unistd.h>

#include "input.hh"
#include "shared.hh"
#include "terminal.hh"

using cchell::input::interactive_input;


namespace
{
    using namespace cchell;


    auto
    handle_input(char ch, bool &reading) -> terminal::decode_status
    {
        auto [status, event] { terminal::decode(ch) };

        if (status != terminal::decode_status::value) return status;

        if (event->code == terminal::key::enter)
        {
            reading = false;
            return terminal::decode_status::none;
        }

        return terminal::decode_status::value;
    }
}


interactive_input *interactive_input::m_instance { nullptr };
interactive_input::interactive_input()
{
    if (shared::tty_status.stdin())
    {
        if (tcgetattr(STDIN_FILENO, &m_old_term) < 0)
            throw std::runtime_error { std::strerror(errno) };

        termios newt { m_old_term };
        newt.c_lflag    &= ~(ICANON | ECHO);
        newt.c_cc[VMIN]  = 1;
        newt.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) < 0)
            throw std::runtime_error { std::strerror(errno) };

        install_sigint_action();
        interactive_input::m_instance = this;
    }
    else
        interactive_input::m_instance = nullptr;
}


void
interactive_input::install_sigint_action()
{
    struct sigaction sa {};
    sa.sa_handler = interactive_input::sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);
}


void
interactive_input::sigint_handler(int sig)
{
    if (sig == SIGINT && interactive_input::m_instance != nullptr)
        interactive_input::m_instance->set_sigint_flag(true);
}


void
interactive_input::set_sigint_flag(bool value)
{
    m_sigint_triggered.store(value, std::memory_order::relaxed);
}


auto
interactive_input::is_sigint_triggered() const noexcept -> bool
{
    return m_sigint_triggered.load(std::memory_order::relaxed);
}


auto
interactive_input::read(std::string &text) noexcept -> int
{
    text.clear();
    set_sigint_flag(false);

    bool reading { true };
    bool escaped { false };
    char ch;
    auto read_status { terminal::decode_status::none };

    while (reading)
    {
        if (::read(STDIN_FILENO, &ch, 1) < 0) return errno;

        /* 0x04 being "End of Transmission" in ASCII */
        if (ch == 0x04) return EOF;

        /* decode status is pending, decoder owns the byte */
        if (read_status == terminal::decode_status::pending)
        {
            read_status = handle_input(ch, reading);
            continue;
        }

        if (!escaped)
        {
            if (ch == '\\')
                escaped = true;
            else
            {
                read_status = handle_input(ch, reading);
                if (read_status != terminal::decode_status::none) continue;
            }

        }
        else
            escaped = false;

        std::fputc(ch, stderr);
        text += ch;
    }

    return 0;
}
