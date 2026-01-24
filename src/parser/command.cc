#include <filesystem>
#include <optional>
#include <ranges>
#include <tuple>
#include <vector>

#include "ask.hh"
#include "diagnostic.hh"
#include "parser.hh"
#include "parser/impl.hh"
#include "shared.hh"

#include "shared.cc" /* NOLINT */

using namespace cchell::parser;
using cchell::diagnostics::diagnostic;
using cchell::diagnostics::diagnostic_builder;
using cchell::diagnostics::severity;


namespace
{
    auto
    is_command(std::string_view data) -> bool
    {
        if (data.empty()) return false;
        std::size_t is_local { data.starts_with("./") ? 1UZ : 0UZ };

        for (std::size_t i { is_local }; i < data.length(); i++)
        {
            char c { data[i] };

            if (is_identifier_char(c)) continue;

            if (is_local != 0 && c == '/') continue;

            if (i > 0 && RESERVED_CHAR.contains(c) && data[i - 1] == '\\')
                continue;

            std::println("{}", c);
            return false;
        }

        return true;
    }


    namespace fs = std::filesystem;

    auto
    directory_view(const fs::path &dir)
    {
        auto dir_it { fs::directory_iterator { dir } };
        return std::views::all(dir_it)
             | std::views::transform([](const fs::directory_entry &e)
                                     { return e.path(); });
    }


    template <std::ranges::input_range T_Range>
    auto
    find_closest_path(const T_Range &paths, const fs::path &target)
        -> std::pair<fs::path, std::size_t>
    {
        auto     smallest { std::numeric_limits<std::size_t>::max() };
        fs::path closest;

        for (const auto &candidate : paths)
        {
            fs::path    filename { candidate.filename() };
            std::size_t dist { cchell::shared::damerau_levenshtein_osa(
                candidate.filename().string(), target.filename().string()) };

            if (dist < smallest)
            {
                closest  = filename;
                smallest = dist;
            }

            if (dist == 0) break;
        }

        return { closest, smallest };
    }


    auto
    find_nearest_looking_path(const fs::path &base) -> std::optional<fs::path>
    {
        std::vector<fs::path> segments;

        for (const auto &part : base) segments.emplace_back(part);
        if (segments.empty()) return std::nullopt;

        std::filesystem::path current_path {
            base.is_absolute() ? base.root_path()
                               : std::filesystem::current_path()
        };

        for (std::size_t i { 0 }; i < segments.size(); i++)
        {
            const fs::path &segment { segments[i] };
            auto            children_view { directory_view(current_path) };

            fs::path    closest;
            std::size_t distance;

            try
            {
                std::tie(closest, distance)
                    = find_closest_path(children_view, segment);
            }
            catch (const fs::filesystem_error &)
            {
                return std::nullopt;
            }

            /* skip if too different */
            std::size_t min_distance { 2 + (segments.size() * 2) };
            if (distance > min_distance) return std::nullopt;

            current_path /= closest;

            /* if not last segment, must be a directory */
            if (i < segments.size() - 1 && !fs::is_directory(current_path))
                return std::nullopt;
        }

        return fs::relative(current_path);
    }


    auto
    handle_path_verification(ast_node &node) -> std::optional<diagnostic>
    {
        fs::path path { node.data.substr(2) }; /* skip the ./ */

        if (fs::exists(path))
        {
            path = fs::canonical(path);

            if (!fs::is_regular_file(path))
                return diagnostic_builder { severity::error }
                    .domain("cchell::parser")
                    .message("path '{}' is not a file", path.string())
                    .annotation(
                        "consider fixing the typo or the directory tree")
                    .source(node.source)
                    .length(node.data.length())
                    .build();

            if (access(path.c_str(), X_OK) != 0)
                return diagnostic_builder { severity::error }
                    .domain("cchell::parser")
                    .message("path '{}' is not an executable", path.string())
                    .annotation("consider changing the permission on '{}'",
                                path.string())
                    .source(node.source)
                    .length(node.data.length())
                    .build();

            return std::nullopt;
        }

        auto err { diagnostic_builder { severity::error }
                       .domain("cchell::parser")
                       .message("executable path '{}' doesn't exist", node.data)
                       .annotation(
                           "consider fixing the typo or the directory tree")
                       .source(node.source)
                       .length(node.data.length())
                       .build() };

        fs::path closest;
        if (auto closest_buff { find_nearest_looking_path(path) };
            !closest_buff)
            return err;
        else /* NOLINT */
            closest = *closest_buff;

        static std::string string_buffer;

        string_buffer += "./" + closest.relative_path().string();
        std::string_view new_data { string_buffer.begin()
                                        + string_buffer.rfind("./"),
                                    string_buffer.end() };


        char response { cchell::ask['y']['n'](
            "executable path '{}' not found, do you mean '{}'?", node.data,
            new_data) };

        if (response != 'y') return err;

        node.set_data(new_data);
        return std::nullopt;
    }
}


auto
impl::command(const lexer::token &token, ast_node &parent) -> bool
{
    if (!is_command(token.data())) return false;

    parent.child.emplace_back(ast_node {}
                                  .set_type(ast_type::command)
                                  .set_source(token.source())
                                  .set_parent(&parent)
                                  .set_data(token.data()));

    return true;
}


auto
impl::verify_command(ast_node &node) -> std::optional<diagnostic>
{
    if (node.data.starts_with("./")) return handle_path_verification(node);

    if (shared::executables.exists(node.data)) return std::nullopt;

    const auto *closest { shared::executables.closest(node.data) };

    if (closest == nullptr
        && ask['y']['n']("command '{}' doesn't exist, do you mean '{}'?",
                         node.data, closest->first)
               != 'y')
        return diagnostic_builder { severity::error }
            .domain("cchell::parser")
            .message("command '{}' doesn't exist", node.data)
            .annotation("consider fixing $PATH or installing the program")
            .source(node.source)
            .length(node.data.length())
            .build();
    node.set_data(closest->first);
    return std::nullopt;
}
