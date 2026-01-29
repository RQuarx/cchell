#pragma once
#include <atomic>
#include <string>

#include <termios.h>


namespace cchell::input
{
    class input_source
    {
    public:
        input_source()          = default;
        virtual ~input_source() = default;

        input_source(input_source &)                     = delete;
        auto operator=(input_source &) -> input_source & = delete;


        /**
         * reads a chunk of @param text from an input source
         *
         * return:
         *    - 0     on success
         *    - errno on error
         *    - EOF   on EOF or EOT
         */
        virtual auto read(std::string &text) noexcept -> int = 0;
    };


    class stream_input : public input_source
    {
    };


    class interactive_input : public input_source
    {
    public:
        interactive_input();

        auto read(std::string &text) noexcept -> int override;

        [[nodiscard]] auto is_sigint_triggered() const noexcept -> bool;

    private:
        termios                   m_old_term;
        static interactive_input *m_instance;
        std::atomic_bool          m_sigint_triggered;


        void set_sigint_flag(bool value);


        static void install_sigint_action();
        static void sigint_handler(int sig);
    };
}
