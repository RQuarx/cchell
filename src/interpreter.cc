#include <algorithm>
#include <cstring>
#include <expected>
#include <memory>
#include <string_view>
#include <vector>

#include <unistd.h>

#include "interpreter.hh"
#include "parser.hh"
#include "shared.hh"


using namespace cchell::diagnostics;
using namespace cchell::parser;
using namespace cchell;


namespace
{
    auto
    ast_to_process(const std::unique_ptr<ast_node> &tree)
        -> std::expected<interpreter::impl::process, std::string>
    {
        interpreter::impl::process proc;

        if (tree->type != ast_type::statement)
            return std::unexpected {
                "the AST's root is not of type \"statement\""
            };

        for (auto &child : tree->child)
        {
            if (child.type == ast_type::assignment)
                proc.envp.emplace_back(child.data);

            if (child.type == ast_type::command)
            {
                if (child.data.starts_with("./"))
                    proc.path = child.data;
                else
                    proc.path = shared::executables.closest(child.data)
                                    ->second.c_str();

                proc.argv.emplace_back(proc.path);
            }

            if (child.type == ast_type::option)
                proc.argv.emplace_back(child.data);
        }

        return proc;
    }
}


auto
interpreter::impl::process::exec() -> bool
{
    std::vector<char *> c_argv { argv.size() + 1 };
    std::vector<char *> c_envp { envp.size() + 1 };

    std::ranges::transform(argv, c_argv.begin(),
                           [](std::string &s) { return s.data(); });
    c_argv.emplace_back(nullptr);
    std::ranges::transform(envp, c_envp.begin(),
                           [](std::string &s) { return s.data(); });
    c_envp.emplace_back(nullptr);

    return execve(path.c_str(), c_argv.data(), c_envp.data()) >= 0;
}


auto
interpreter::execute(const std::unique_ptr<ast_node> &tree)
    -> std::expected<pid_t, std::string>
{
    impl::process proc;

    if (auto buf { ast_to_process(tree) }; !buf)
        return std::unexpected { buf.error() };
    else /* NOLINT: clangd doesnt understand that
           `buf` is scoped to the if-else */
        proc = *buf;

    pid_t pid { fork() };

    if (pid == 0)
        if (!proc.exec()) return std::unexpected { std::strerror(errno) };
    return pid;
}
