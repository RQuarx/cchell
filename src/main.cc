#include <format>
#include <memory>
#include <print>
#include <string>
#include <utility>

#include <lyra/lyra.hpp>

#include "ask.hh"
#include "diagnostic.hh"
#include "lexer.hh"
#include "parser.hh"


namespace
{
    [[noreturn]]
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

        std::exit(0);
    }


    [[noreturn]]
    void
    print_version()
    {
        std::println("{} {}", PROJECT_NAME, PROJECT_VERSION);
        std::exit(0);
    }


    [[nodiscard]]
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
}


auto
main(int argc, char **argv) -> int
{
    std::string commands { get_commands(argc, argv) };

    bool show_help { false };
    bool show_version { false };

    /* clang-format off */
    auto cli { lyra::cli {}
             | lyra::opt { show_version }["-V"]["--version"]
             | lyra::help { show_help } };
    /* clang-format on */

    if (auto res { cli.parse({ argc, argv }) }; !res)
        return std::println("{}", res.message()), 1;

    if (show_help) print_help(*argv);
    if (show_version) print_version();

    std::println("{}", commands.empty() ? "empty" : commands);
    if (commands.empty()) return 0;

    auto tokens { cchell::lexer::lex(commands) };
    std::println("{}", tokens);

    std::unique_ptr<cchell::parser::ast_node> ast;

    try
    {
        ast = cchell::parser::parse(tokens);
    }
    catch (cchell::diagnostic &diag)
    {
        std::print("{}", diag.set_raw(commands).format());
        return 1;
    }

    std::println("{}", *ast);
    if (auto diag {cchell::parser::verify(*ast) })
        std::print("{}", diag->set_raw(commands).format());
    else
        std::println("{}", *ast);

    return 0;
}
