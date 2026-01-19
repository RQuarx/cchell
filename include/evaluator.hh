#pragma once
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>


namespace cchell::parser { struct ast_node; }


namespace cchell::evaluator
{
    class process
    {
    public:
        process(std::unique_ptr<parser::ast_node> &&nodes);


        auto run() -> int;
        void wait();

    private:
        std::unique_ptr<parser::ast_node> m_ast;

        std::unordered_map<std::string, std::string> m_env;

        pid_t m_pid { -1 };
        int   m_exit_code { 0 };
        bool  m_background { false };

        std::optional<std::string> stdin_file;
        std::optional<std::string> stdout_file;
        std::optional<std::string> stderr_file;
        bool                       append_stdout { false };
    };
}
