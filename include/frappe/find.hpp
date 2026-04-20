#ifndef FRAPPE_FIND_HPP
#define FRAPPE_FIND_HPP

#include "frappe/exports.hpp"

#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace frappe {

using path = std::filesystem::path;
using file_time_type = std::filesystem::file_time_type;

class FRAPPE_API find_options {
  public:
    find_options() = default;

    // Name matching
    find_options &name(std::string_view pattern);
    find_options &iname(std::string_view pattern);
    find_options &regex(std::string_view pattern);
    find_options &iregex(std::string_view pattern);
    find_options &path_regex(std::string_view pattern);
    find_options &extension(std::string_view ext);
    find_options &extensions(std::initializer_list<std::string_view> exts);

    // Type filtering
    find_options &type(std::filesystem::file_type ft);
    find_options &is_file();
    find_options &is_dir();
    find_options &is_symlink();
    find_options &is_block();
    find_options &is_char();
    find_options &is_fifo();
    find_options &is_socket();

    // Size filtering
    find_options &size_eq(std::uintmax_t size);
    find_options &size_gt(std::uintmax_t size);
    find_options &size_lt(std::uintmax_t size);
    find_options &size_range(std::uintmax_t min_size, std::uintmax_t max_size);
    find_options &empty();

    // Time filtering
    find_options &mtime_newer(file_time_type time);
    find_options &mtime_older(file_time_type time);
    find_options &mtime_range(file_time_type start, file_time_type end);
    find_options &mmin(int minutes);
    find_options &mtime(int days);
    find_options &newer_than(const path &p);
    find_options &older_than(const path &p);

    // Permission filtering
    find_options &readable();
    find_options &writable();
    find_options &executable();

    // Depth control
    find_options &max_depth(int depth);
    find_options &min_depth(int depth);
    find_options &depth_range(int min_d, int max_d);

    // Link filtering
    find_options &links_eq(std::uintmax_t count);
    find_options &links_gt(std::uintmax_t count);
    find_options &links_lt(std::uintmax_t count);
    find_options &broken_symlink();

    // Filesystem filtering
    find_options &xdev();

    // Custom predicate
    find_options &predicate(std::function<bool(const std::filesystem::directory_entry &)> pred);

    // Check if entry matches all conditions
    [[nodiscard]] bool matches(const std::filesystem::directory_entry &entry, int current_depth = 0) const;

    // Getters
    [[nodiscard]] int get_max_depth() const noexcept { return _max_depth; }
    [[nodiscard]] int get_min_depth() const noexcept { return _min_depth; }
    [[nodiscard]] bool get_xdev() const noexcept { return _xdev; }

  private:
    struct condition {
        std::function<bool(const std::filesystem::directory_entry &)> predicate;
    };

    std::vector<condition> _conditions;
    int _max_depth = -1;
    int _min_depth = 0;
    bool _xdev = false;
};

// Search execution
[[nodiscard]] FRAPPE_API std::vector<path> find(const path &search_path, const find_options &options = {});
[[nodiscard]] FRAPPE_API std::vector<path> find(const path &search_path, const find_options &options,
                                                std::error_code &ec);

// Lazy search - returns a generator/range
class FRAPPE_API find_iterator {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type = path;
    using difference_type = std::ptrdiff_t;
    using pointer = const path *;
    using reference = const path &;

    find_iterator() = default;
    find_iterator(const path &search_path, const find_options &options);

    reference operator*() const;
    pointer operator->() const;
    find_iterator &operator++();
    find_iterator operator++(int);

    bool operator==(const find_iterator &other) const;
    bool operator!=(const find_iterator &other) const;

  private:
    class impl;
    std::shared_ptr<impl> _impl;
};

class FRAPPE_API find_range {
  public:
    find_range(const path &search_path, const find_options &options = {});

    [[nodiscard]] find_iterator begin() const;
    [[nodiscard]] find_iterator end() const;

  private:
    path _search_path;
    find_options _options;
};

[[nodiscard]] inline find_range find_lazy(const path &search_path, const find_options &options = {}) {
    return find_range(search_path, options);
}

// Find first match
[[nodiscard]] FRAPPE_API std::optional<path> find_first(const path &search_path, const find_options &options);

// Count matches
[[nodiscard]] FRAPPE_API std::size_t find_count(const path &search_path, const find_options &options);

// Size literals for convenience
namespace literals {
constexpr std::uintmax_t operator""_B(unsigned long long v) {
    return v;
}
constexpr std::uintmax_t operator""_KB(unsigned long long v) {
    return v * 1024;
}
constexpr std::uintmax_t operator""_MB(unsigned long long v) {
    return v * 1024 * 1024;
}
constexpr std::uintmax_t operator""_GB(unsigned long long v) {
    return v * 1024 * 1024 * 1024;
}
constexpr std::uintmax_t operator""_TB(unsigned long long v) {
    return v * 1024ULL * 1024 * 1024 * 1024;
}
} // namespace literals

// Result processing adapters
struct sort_by_name_fn {
    template <std::ranges::input_range R> auto operator()(R &&r) const {
        std::vector<std::ranges::range_value_t<R>> result;
        for (auto &&item : r) {
            result.push_back(item);
        }
        std::ranges::sort(result, [](const auto &a, const auto &b) { return a.filename() < b.filename(); });
        return result;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const sort_by_name_fn &fn) {
        return fn(std::forward<R>(r));
    }
};

struct sort_by_size_fn {
    template <std::ranges::input_range R> auto operator()(R &&r) const {
        std::vector<std::ranges::range_value_t<R>> result;
        for (auto &&item : r) {
            result.push_back(item);
        }
        std::ranges::sort(result, [](const auto &a, const auto &b) {
            std::error_code ec;
            auto sa = std::filesystem::file_size(a, ec);
            auto sb = std::filesystem::file_size(b, ec);
            return sa < sb;
        });
        return result;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const sort_by_size_fn &fn) {
        return fn(std::forward<R>(r));
    }
};

struct sort_by_mtime_fn {
    bool descending = false;

    template <std::ranges::input_range R> auto operator()(R &&r) const {
        std::vector<std::ranges::range_value_t<R>> result;
        for (auto &&item : r) {
            result.push_back(item);
        }
        if (descending) {
            std::ranges::sort(result, [](const auto &a, const auto &b) {
                std::error_code ec;
                auto ta = std::filesystem::last_write_time(a, ec);
                auto tb = std::filesystem::last_write_time(b, ec);
                return ta > tb; // Descending: newer first
            });
        } else {
            std::ranges::sort(result, [](const auto &a, const auto &b) {
                std::error_code ec;
                auto ta = std::filesystem::last_write_time(a, ec);
                auto tb = std::filesystem::last_write_time(b, ec);
                return ta < tb; // Ascending: older first
            });
        }
        return result;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const sort_by_mtime_fn &fn) {
        return fn(std::forward<R>(r));
    }
};

struct sort_by_extension_fn {
    template <std::ranges::input_range R> auto operator()(R &&r) const {
        std::vector<std::ranges::range_value_t<R>> result;
        for (auto &&item : r) {
            result.push_back(item);
        }
        std::ranges::sort(result, [](const auto &a, const auto &b) { return a.extension() < b.extension(); });
        return result;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const sort_by_extension_fn &fn) {
        return fn(std::forward<R>(r));
    }
};

inline constexpr sort_by_name_fn sort_by_name{};
inline constexpr sort_by_size_fn sort_by_size{};
inline constexpr sort_by_mtime_fn sort_by_mtime{};
inline constexpr sort_by_mtime_fn sort_by_mtime_desc{true};
inline constexpr sort_by_extension_fn sort_by_extension{};

// Take first N elements
struct take_fn {
    std::size_t count;

    template <std::ranges::input_range R> auto operator()(R &&r) const {
        std::vector<std::ranges::range_value_t<R>> result;
        result.reserve(count);
        std::size_t i = 0;
        for (auto &&item : r) {
            if (i++ >= count) break;
            result.push_back(std::forward<decltype(item)>(item));
        }
        return result;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const take_fn &fn) {
        return fn(std::forward<R>(r));
    }
};

[[nodiscard]] inline take_fn take(std::size_t n) {
    return take_fn{n};
}

// Take first element
struct first_fn {
    template <std::ranges::input_range R> auto operator()(R &&r) const -> std::optional<std::ranges::range_value_t<R>> {
        auto it = std::ranges::begin(r);
        if (it != std::ranges::end(r)) {
            return *it;
        }
        return std::nullopt;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const first_fn &fn) {
        return fn(std::forward<R>(r));
    }
};

inline constexpr first_fn first{};

// Reverse order
struct reverse_fn {
    template <std::ranges::input_range R> auto operator()(R &&r) const {
        std::vector<std::ranges::range_value_t<R>> result;
        for (auto &&item : r) {
            result.push_back(item);
        }
        std::ranges::reverse(result);
        return result;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const reverse_fn &fn) {
        return fn(std::forward<R>(r));
    }
};

inline constexpr reverse_fn reverse{};

// Filter by filename regex (convenience wrapper)
struct filter_filename_regex_fn {
    std::string pattern;

    template <std::ranges::input_range R> auto operator()(R &&r) const {
        return std::forward<R>(r) | std::views::filter([pat = this->pattern](const auto &p) {
                   path filepath;
                   if constexpr (std::is_convertible_v<decltype(p), path>) {
                       filepath = p;
                   } else {
                       filepath = p.path();
                   }
                   // Simple regex match on filename only
                   std::string filename = filepath.filename().string();
                   // Use basic pattern matching (for full regex, use filter_regex)
                   return filename.find(pat) != std::string::npos;
               });
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const filter_filename_regex_fn &fn) {
        return fn(std::forward<R>(r));
    }
};

[[nodiscard]] inline filter_filename_regex_fn filter_filename(std::string pattern) {
    return filter_filename_regex_fn{std::move(pattern)};
}

// Total size calculation
[[nodiscard]] FRAPPE_API std::uintmax_t total_size(const std::vector<path> &paths);

template <std::ranges::input_range R> [[nodiscard]] std::uintmax_t total_size(R &&r) {
    std::uintmax_t total = 0;
    std::error_code ec;
    for (const auto &p : r) {
        if (std::filesystem::is_regular_file(p, ec)) {
            total += std::filesystem::file_size(p, ec);
        }
    }
    return total;
}

} // namespace frappe

#endif // FRAPPE_FIND_HPP
