#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

#include <unistd.h>

#include "shared.hh"

using namespace cchell::shared;


namespace
{
    auto
    is_executable(const char *name) -> bool
    {
        return access(name, X_OK) == 0;
    }
}


auto
cchell::shared::damerau_levenshtein_osa(std::string_view a, std::string_view b)
    -> std::size_t
{
    const std::size_t n { a.size() };
    const std::size_t m { b.size() };

    if (n == 0) return m;
    if (m == 0) return n;

    std::vector<std::size_t> prev2;
    std::vector<std::size_t> curr;
    prev2.resize(m + 1);
    curr.resize(m + 1);

    auto prev { std::views::iota(0UZ, m + 1) | std::ranges::to<std::vector>() };

    for (std::size_t i { 1 }; i <= n; i++)
    {
        curr[0] = i;

        for (std::size_t j { 1 }; j <= m; ++j)
        {
            std::size_t cost { a[i - 1] == b[j - 1] ? 0UZ : 1UZ };

            curr[j] = std::min(
                { prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost });

            if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1])
                curr[j] = std::min(curr[j], prev2[j - 2] + 1);
        }

        prev2 = prev;
        prev  = curr;
    }

    return prev[m];
}


impl::executables::executables()
{
    const char *CPATH { std::getenv("PATH") };
    if (CPATH == nullptr) throw std::runtime_error { "$PATH is not defined." };
    std::string_view PATH { CPATH };

    for (auto subrange : PATH | std::views::split(':'))
    {
        std::string_view directory { &*subrange.begin(),
                                     static_cast<std::size_t>(
                                         std::ranges::distance(subrange)) };

        if (directory.empty()) continue;
        if (!std::filesystem::exists(directory)
            || !std::filesystem::is_directory(directory))
            continue;

        for (const auto &dir :
             std::filesystem::directory_iterator { directory })
        {
            const auto &path { dir.path() };

            if (!is_executable(path.c_str())) continue;

            m_paths.emplace(path.filename().string(),
                            std::filesystem::canonical(path));
        }
    }
}


auto
impl::executables::exists(std::string_view name) const -> bool
{
    return m_paths.contains(name);
}


auto
impl::executables::closest(std::string_view name,
                           std::size_t      max_distance) const
    -> const std::pair<const std::string, std::filesystem::path> *
{
    if (m_paths.empty()) throw std::runtime_error { "no executables indexed" };

    const std::pair<const std::string, std::filesystem::path> *closest {
        nullptr
    };
    auto closest_distance { std::numeric_limits<std::size_t>::max() };

    for (const auto &entry : m_paths)
    {
        const std::string &cmd { entry.first };

        if (std::abs(static_cast<int>(cmd.length() - name.length())) > 2)
            continue;

        std::size_t dist { damerau_levenshtein_osa(name, cmd) };

        if (dist < closest_distance)
        {
            closest_distance = dist;
            closest          = &entry;
        }
    }

    if (closest == nullptr || closest_distance > max_distance) return nullptr;

    return closest;
}


impl::tty_status::tty_status() : m_ttys { 0 }
{
    if (isatty(STDIN_FILENO) == 1) m_ttys |= std::to_underlying(bits::stdin);
    if (isatty(STDOUT_FILENO) == 1) m_ttys |= std::to_underlying(bits::stdout);
    if (isatty(STDERR_FILENO) == 1) m_ttys |= std::to_underlying(bits::stderr);
}


auto
impl::tty_status::stdin() const -> bool
{
    return (m_ttys & std::to_underlying(bits::stdin)) != 0;
}


auto
impl::tty_status::stdout() const -> bool
{
    return (m_ttys & std::to_underlying(bits::stdout)) != 0;
}


auto
impl::tty_status::stderr() const -> bool
{
    return (m_ttys & std::to_underlying(bits::stderr)) != 0;
}

auto
impl::tty_status::operator[](unsigned int fd) const -> bool
{
    if (fd == 0) return stdin();
    if (fd == 1) return stdout();
    if (fd == 2) return stderr();
    return isatty(fd) == 1;
}
