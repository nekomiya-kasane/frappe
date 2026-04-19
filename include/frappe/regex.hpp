#ifndef FRAPPE_REGEX_HPP
#define FRAPPE_REGEX_HPP

#include "frappe/error.hpp"
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>
#include <ranges>

namespace frappe {

using path = std::filesystem::path;

enum class regex_options : unsigned {
    none = 0,
    caseless = 1 << 0,
    multiline = 1 << 1,
    dotall = 1 << 2,
    extended = 1 << 3,
    utf = 1 << 4,
};

constexpr regex_options operator|(regex_options a, regex_options b) noexcept {
    return static_cast<regex_options>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}

constexpr regex_options operator&(regex_options a, regex_options b) noexcept {
    return static_cast<regex_options>(static_cast<unsigned>(a) & static_cast<unsigned>(b));
}

constexpr bool has_option(regex_options opts, regex_options flag) noexcept {
    return (static_cast<unsigned>(opts) & static_cast<unsigned>(flag)) != 0;
}

class FRAPPE_API compiled_regex {
public:
    compiled_regex();
    explicit compiled_regex(std::string_view pattern, regex_options opts = regex_options::none);
    ~compiled_regex();

    compiled_regex(const compiled_regex&) = delete;
    compiled_regex& operator=(const compiled_regex&) = delete;
    compiled_regex(compiled_regex&& other) noexcept;
    compiled_regex& operator=(compiled_regex&& other) noexcept;

    [[nodiscard]] bool match(std::string_view subject) const;
    [[nodiscard]] bool search(std::string_view subject) const;
    [[nodiscard]] std::optional<std::string> replace(std::string_view subject, std::string_view replacement) const;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] std::string_view pattern() const noexcept;

private:
    class impl;
    std::unique_ptr<impl> _impl;
};

[[nodiscard]] FRAPPE_API compiled_regex compile_regex(std::string_view pattern, regex_options opts = regex_options::none);

[[nodiscard]] FRAPPE_API bool regex_match(const path& p, std::string_view pattern);
[[nodiscard]] FRAPPE_API bool regex_match(std::string_view name, std::string_view pattern);
[[nodiscard]] FRAPPE_API bool regex_match(const path& p, const compiled_regex& re);
[[nodiscard]] FRAPPE_API bool regex_match(std::string_view name, const compiled_regex& re);

[[nodiscard]] FRAPPE_API bool regex_search(const path& p, std::string_view pattern);
[[nodiscard]] FRAPPE_API bool regex_search(std::string_view text, std::string_view pattern);
[[nodiscard]] FRAPPE_API bool regex_search(const path& p, const compiled_regex& re);
[[nodiscard]] FRAPPE_API bool regex_search(std::string_view text, const compiled_regex& re);

[[nodiscard]] FRAPPE_API std::optional<path> regex_replace(const path& p, std::string_view pattern, std::string_view replacement);
[[nodiscard]] FRAPPE_API std::optional<std::string> regex_replace(std::string_view text, std::string_view pattern, std::string_view replacement);
[[nodiscard]] FRAPPE_API std::optional<path> regex_replace(const path& p, const compiled_regex& re, std::string_view replacement);
[[nodiscard]] FRAPPE_API std::optional<std::string> regex_replace(std::string_view text, const compiled_regex& re, std::string_view replacement);

struct filter_regex_fn {
    std::string pattern;

    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        auto re = std::make_shared<compiled_regex>(pattern);
        return std::forward<R>(r) | std::views::filter([re](const auto& p) {
            if constexpr (std::is_convertible_v<decltype(p), path>) {
                return re->match(path(p).string());
            } else {
                return re->match(std::string(p));
            }
        });
    }

    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const filter_regex_fn& fn) {
        return fn(std::forward<R>(r));
    }
};

[[nodiscard]] inline filter_regex_fn filter_regex(std::string pattern) {
    return filter_regex_fn{std::move(pattern)};
}

struct filter_regex_not_fn {
    std::string pattern;

    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        auto re = std::make_shared<compiled_regex>(pattern);
        return std::forward<R>(r) | std::views::filter([re](const auto& p) {
            if constexpr (std::is_convertible_v<decltype(p), path>) {
                return !re->match(path(p).string());
            } else {
                return !re->match(std::string(p));
            }
        });
    }

    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const filter_regex_not_fn& fn) {
        return fn(std::forward<R>(r));
    }
};

[[nodiscard]] inline filter_regex_not_fn filter_regex_not(std::string pattern) {
    return filter_regex_not_fn{std::move(pattern)};
}

} // namespace frappe

#endif // FRAPPE_REGEX_HPP
