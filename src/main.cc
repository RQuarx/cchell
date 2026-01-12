#include <print>
#include <string>

#include <lyra/lyra.hpp>


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
}


auto
main(int argc, char **argv) -> int
{
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

    return 0;
}
