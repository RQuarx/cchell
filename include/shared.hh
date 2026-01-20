#pragma once
#include <array>
#include <filesystem>
#include <string>
#include <unordered_map>


namespace cchell::shared
{
    namespace impl
    {
        class executables
        {
            struct string_hash
            {
                using is_transparent = void;

                auto
                operator()(std::string_view sv) const noexcept -> std::size_t
                {
                    return std::hash<std::string_view> {}(sv);
                }

                auto
                operator()(const std::string &s) const noexcept -> std::size_t
                {
                    return std::hash<std::string_view> {}(s);
                }
            };

            struct string_equal
            {
                using is_transparent = void;

                auto
                operator()(std::string_view a,
                           std::string_view b) const noexcept -> bool
                {
                    return a == b;
                }
            };

        public:
            executables();


            [[nodiscard]]
            auto exists(std::string_view name) const -> bool;


            [[nodiscard]]
            auto closest(std::string_view name,
                         std::size_t      max_distance = 2) const
                -> const std::pair<const std::string, std::filesystem::path> *;

        private:
            std::unordered_map<std::string,
                               std::filesystem::path,
                               string_hash,
                               string_equal>
                m_paths;
        };


        class tty_status
        {
        public:
            enum class bits : std::uint8_t
            {
                none   = 0,
                stdin  = 1 << 0,
                stdout = 1 << 1,
                stderr = 1 << 2,
            };

            tty_status();

            auto stdin() const -> bool;
            auto stdout() const -> bool;
            auto stderr() const -> bool;


            auto operator[](unsigned int fd) const -> bool;

        private:
            std::uint8_t m_ttys;
        };
    }


    [[nodiscard]]
    auto damerau_levenshtein_osa(std::string_view a, std::string_view b)
        -> std::size_t;


    inline impl::tty_status                                       tty_status;
    inline impl::executables                                      executables;
    inline std::unordered_map<std::string_view, std::string_view> envp;
}
