#pragma once
#include <expected>
#include <memory>
#include <string>
#include <vector>


namespace cchell::parser { struct ast_node; }
namespace cchell::diagnostics { struct diagnostic; }


namespace cchell::interpreter
{
    namespace impl
    {
        struct process
        {
            std::vector<std::string> envp;

            std::string              path;
            std::vector<std::string> argv;


            auto exec() -> bool;
        };
    }


    [[nodiscard]]
    auto execute(const std::unique_ptr<parser::ast_node> &tree)
        -> std::expected<pid_t, std::string>;
}
