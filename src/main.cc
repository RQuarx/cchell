#include <format>
#include <memory>
#include <print>
#include <string>
#include <string_view>

#include <lyra/lyra.hpp>

#include "diagnostic.hh"
#include "parser.hh"
#include "shared.hh"


namespace
{
    void
    print_help(std::string_view binary_name)
    {
        /* clang-format off */
        std::print(
"Usage: {} [options {{params}}] -- <commands>\n"
"\n"
"Options:\n"
"  -h --help                    Show this message.\n"
"  -V --version                 Show version info.\n"
, binary_name
        );
        /* clang-format on */
    }


    void
    print_version()
    {
        std::println("{} {}", PROJECT_NAME, PROJECT_VERSION);
    }


    auto
    get_commands(int &argc, char **argv) -> std::string
    {
        std::string commands;

        for (auto i { 1 }; i < argc; i++)
        {
            std::string_view arg { argv[i] };

            if (arg != "--") continue;

            for (int j { i + 1 }; j < argc; j++)
            {
                commands += argv[j];
                if (j != argc - 1) commands += ' ';
            }

            argc = i;
            break;
        }

        return commands;
    }


    void
    fill_global_env(char **envp)
    {
        for (char **p { envp }; *p != nullptr; p++)
        {
            std::string_view env { *p };

            if (auto idx { env.find('=') }; idx != std::string_view::npos)
                cchell::shared::envp.emplace(env.substr(0, idx),
                                             env.substr(idx + 1));
        }
    }
}


auto
main(int argc, char **argv, char **envp) -> int
{
    fill_global_env(envp);
    std::string commands { get_commands(argc, argv) };

    bool show_help { false };
    bool show_version { false };

    /* clang-format off */
    auto cli { lyra::cli {}
             | lyra::opt { show_version }["-V"]["--version"]
             | lyra::help { show_help } };
    /* clang-format on */

    std::println("{}", cchell::shared::tty_status.stdin());

    if (auto res { cli.parse({ argc, argv }) }; !res)
        return std::println("{}", res.message()), 1;

    if (show_help) return print_help(*argv), 0;
    if (show_version) return print_version(), 0;

    std::println("{}", commands.empty() ? "empty" : commands);
    if (commands.empty()) return 0;

    auto tokens { cchell::lexer::lex(commands) };
    std::println("{}", tokens);

    std::unique_ptr<cchell::parser::ast_node> ast;

    try
    {
        ast = cchell::parser::parse(tokens);
    }
    catch (cchell::diagnostics::diagnostic &diag)
    {
        std::print("{}", diag.render(commands, "argv"));
        return 1;
    }

    std::println("{}", *ast);
    if (auto diag { cchell::parser::verify(*ast) })
        std::print("{}", diag->render(commands, "argv"));
    else
        std::println("{}", *ast);

    return 0;
}
