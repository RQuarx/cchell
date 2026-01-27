#pragma once
#include <atomic>
#include <expected>
#include <string>

#include <termios.h>


namespace cchell
{
    class input
    {
    public:
        [[nodiscard]]
        static auto create() noexcept -> std::expected<input, std::string>;


        input(input &)                     = delete;
        auto operator=(input &) -> input & = delete;

        input(input &&other) noexcept;
        auto operator=(input &&other) noexcept -> input &;


        /* returns 0 on success, `errno` on error, and EOF (-1) on EOF. */
        auto read(std::string &text) noexcept -> int;

    private:
        termios       m_old_term;
        static input *m_instance;

        volatile std::atomic_bool m_sigint_triggered;


        input();


        auto read_stdin(std::string &text) noexcept -> int;


        static void install_sigint_action();
        static void sigint_handler(int sig);
    };
}
