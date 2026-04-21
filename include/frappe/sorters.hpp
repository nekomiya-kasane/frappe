#ifndef FRAPPE_SORTERS_HPP
#define FRAPPE_SORTERS_HPP

#include "frappe/entry.hpp"
#include "frappe/projections.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <ranges>
#include <vector>

namespace frappe {

    // ============================================================================
    // Generic sort adapter
    // ============================================================================

    template <typename Proj, typename Comp = std::less<>> struct sort_adapter {
        Proj projection;
        Comp comparator;
        bool descending = false;

        template <std::ranges::input_range R> auto operator()(R &&r) const {
            std::vector<std::ranges::range_value_t<R>> result;
            for (auto &&item : r) {
                result.push_back(std::forward<decltype(item)>(item));
            }

            if (descending) {
                std::ranges::sort(
                    result, [this](const auto &a, const auto &b) { return comparator(projection(b), projection(a)); });
            } else {
                std::ranges::sort(
                    result, [this](const auto &a, const auto &b) { return comparator(projection(a), projection(b)); });
            }
            return result;
        }

        template <std::ranges::input_range R> friend auto operator|(R &&r, const sort_adapter &s) {
            return s(std::forward<R>(r));
        }

        // Create descending version
        [[nodiscard]] auto desc() const { return sort_adapter<Proj, Comp>{projection, comparator, true}; }

        // Create ascending version
        [[nodiscard]] auto asc() const { return sort_adapter<Proj, Comp>{projection, comparator, false}; }
    };

    template <typename Proj, typename Comp = std::less<>> [[nodiscard]] inline auto sort_by(Proj proj, Comp comp = {}) {
        return sort_adapter<Proj, Comp>{std::move(proj), std::move(comp), false};
    }

    template <typename Proj, typename Comp = std::less<>>
    [[nodiscard]] inline auto sort_by_desc(Proj proj, Comp comp = {}) {
        return sort_adapter<Proj, Comp>{std::move(proj), std::move(comp), true};
    }

    // ============================================================================
    // Predefined sorters for file_entry
    // ============================================================================

    // Predefined sorters for file_entry (in entry namespace to avoid conflicts with find.hpp)
    namespace entry {
        // Name sorting
        inline const auto sort_by_name = sort_by(proj::name);
        inline const auto sort_by_name_desc = sort_by_desc(proj::name);
        inline const auto sort_by_name_icase = sort_by(proj::name, [](const std::string &a, const std::string &b) {
            return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
                                                [](char c1, char c2) { return std::tolower(c1) < std::tolower(c2); });
        });

        // Size sorting
        inline const auto sort_by_size = sort_by(proj::size);
        inline const auto sort_by_size_desc = sort_by_desc(proj::size);

        // Time sorting
        inline const auto sort_by_mtime = sort_by(proj::mtime);
        inline const auto sort_by_mtime_desc = sort_by_desc(proj::mtime);
        inline const auto sort_by_atime = sort_by(proj::atime);
        inline const auto sort_by_atime_desc = sort_by_desc(proj::atime);
        inline const auto sort_by_ctime = sort_by(proj::ctime);
        inline const auto sort_by_ctime_desc = sort_by_desc(proj::ctime);
        inline const auto sort_by_birth_time = sort_by(proj::birth_time);
        inline const auto sort_by_birth_time_desc = sort_by_desc(proj::birth_time);

        // Extension sorting
        inline const auto sort_by_extension = sort_by(proj::extension);
        inline const auto sort_by_extension_desc = sort_by_desc(proj::extension);

        // Type sorting
        inline const auto sort_by_type = sort_by(proj::type);

        // Owner sorting
        inline const auto sort_by_owner = sort_by(proj::owner);
        inline const auto sort_by_group = sort_by(proj::group);

        // Inode sorting
        inline const auto sort_by_inode = sort_by(proj::inode);

        // Path sorting
        inline const auto sort_by_path = sort_by(proj::file_path);
        inline const auto sort_by_path_desc = sort_by_desc(proj::file_path);
    } // namespace entry

    // ============================================================================
    // Multi-level sorting
    // ============================================================================

    template <typename... Projs> struct multi_sort_adapter {
        std::tuple<Projs...> projections;

        template <std::ranges::input_range R> auto operator()(R &&r) const {
            std::vector<std::ranges::range_value_t<R>> result;
            for (auto &&item : r) {
                result.push_back(std::forward<decltype(item)>(item));
            }

            std::ranges::sort(result, [this](const auto &a, const auto &b) {
                return compare_multi(a, b, std::index_sequence_for<Projs...>{});
            });
            return result;
        }

        template <std::ranges::input_range R> friend auto operator|(R &&r, const multi_sort_adapter &s) {
            return s(std::forward<R>(r));
        }

      private:
        template <typename T, std::size_t... Is>
        bool compare_multi(const T &a, const T &b, std::index_sequence<Is...>) const {
            bool result = false;
            bool decided = false;
            ((decided || (std::get<Is>(projections)(a) != std::get<Is>(projections)(b)
                              ? (result = std::get<Is>(projections)(a) < std::get<Is>(projections)(b), decided = true)
                              : false)),
             ...);
            return result;
        }
    };

    template <typename... Projs> [[nodiscard]] inline auto sort_by_multi(Projs... projs) {
        return multi_sort_adapter<Projs...>{std::make_tuple(std::move(projs)...)};
    }

    // ============================================================================
    // Stable sort adapter
    // ============================================================================

    template <typename Proj, typename Comp = std::less<>> struct stable_sort_adapter {
        Proj projection;
        Comp comparator;
        bool descending = false;

        template <std::ranges::input_range R> auto operator()(R &&r) const {
            std::vector<std::ranges::range_value_t<R>> result;
            for (auto &&item : r) {
                result.push_back(std::forward<decltype(item)>(item));
            }

            if (descending) {
                std::ranges::stable_sort(
                    result, [this](const auto &a, const auto &b) { return comparator(projection(b), projection(a)); });
            } else {
                std::ranges::stable_sort(
                    result, [this](const auto &a, const auto &b) { return comparator(projection(a), projection(b)); });
            }
            return result;
        }

        template <std::ranges::input_range R> friend auto operator|(R &&r, const stable_sort_adapter &s) {
            return s(std::forward<R>(r));
        }
    };

    template <typename Proj, typename Comp = std::less<>>
    [[nodiscard]] inline auto stable_sort_by(Proj proj, Comp comp = {}) {
        return stable_sort_adapter<Proj, Comp>{std::move(proj), std::move(comp), false};
    }

    template <typename Proj, typename Comp = std::less<>>
    [[nodiscard]] inline auto stable_sort_by_desc(Proj proj, Comp comp = {}) {
        return stable_sort_adapter<Proj, Comp>{std::move(proj), std::move(comp), true};
    }

    // ============================================================================
    // Reverse adapter (in entry namespace to avoid conflict with find.hpp)
    // ============================================================================

    namespace entry {
        struct reverse_adapter {
            template <std::ranges::input_range R> auto operator()(R &&r) const {
                std::vector<std::ranges::range_value_t<R>> result;
                for (auto &&item : r) {
                    result.push_back(std::forward<decltype(item)>(item));
                }
                std::ranges::reverse(result);
                return result;
            }

            template <std::ranges::input_range R> friend auto operator|(R &&r, const reverse_adapter &rev) {
                return rev(std::forward<R>(r));
            }
        };

        inline constexpr reverse_adapter reverse{};
    } // namespace entry

    // ============================================================================
    // Natural sorting (number-aware string comparison)
    // ============================================================================

    namespace detail {
        // Extract numeric value from string starting at pos
        inline std::pair<std::uintmax_t, std::size_t> extract_number(const std::string &s, std::size_t pos) {
            std::uintmax_t num = 0;
            std::size_t len = 0;
            while (pos + len < s.size() && std::isdigit(static_cast<unsigned char>(s[pos + len]))) {
                num = num * 10 + (s[pos + len] - '0');
                ++len;
            }
            return {num, len};
        }

        // Natural comparison: "file2" < "file10"
        inline bool natural_less(const std::string &a, const std::string &b) {
            std::size_t i = 0, j = 0;
            while (i < a.size() && j < b.size()) {
                bool a_digit = std::isdigit(static_cast<unsigned char>(a[i]));
                bool b_digit = std::isdigit(static_cast<unsigned char>(b[j]));

                if (a_digit && b_digit) {
                    auto [num_a, len_a] = extract_number(a, i);
                    auto [num_b, len_b] = extract_number(b, j);
                    if (num_a != num_b) {
                        return num_a < num_b;
                    }
                    i += len_a;
                    j += len_b;
                } else {
                    unsigned char ca = static_cast<unsigned char>(std::tolower(a[i]));
                    unsigned char cb = static_cast<unsigned char>(std::tolower(b[j]));
                    if (ca != cb) {
                        return ca < cb;
                    }
                    ++i;
                    ++j;
                }
            }
            return a.size() < b.size();
        }
    } // namespace detail

    struct natural_sort_adapter {
        bool descending = false;

        template <std::ranges::input_range R> auto operator()(R &&r) const {
            std::vector<std::ranges::range_value_t<R>> result;
            for (auto &&item : r) {
                result.push_back(std::forward<decltype(item)>(item));
            }

            if (descending) {
                std::ranges::sort(result,
                                  [](const auto &a, const auto &b) { return detail::natural_less(b.name, a.name); });
            } else {
                std::ranges::sort(result,
                                  [](const auto &a, const auto &b) { return detail::natural_less(a.name, b.name); });
            }
            return result;
        }

        template <std::ranges::input_range R> friend auto operator|(R &&r, const natural_sort_adapter &s) {
            return s(std::forward<R>(r));
        }

        [[nodiscard]] auto desc() const { return natural_sort_adapter{true}; }
    };

    inline constexpr natural_sort_adapter sort_natural{};
    inline constexpr natural_sort_adapter sort_natural_desc{true};

    // ============================================================================
    // Additional sorters in entry namespace
    // ============================================================================

    namespace entry {
        // Depth sorting
        inline const auto sort_by_depth =
            sort_by([](const file_entry &e) { return std::distance(e.file_path.begin(), e.file_path.end()); });
        inline const auto sort_by_depth_desc =
            sort_by_desc([](const file_entry &e) { return std::distance(e.file_path.begin(), e.file_path.end()); });

        // Parent path sorting
        inline const auto sort_by_parent = sort_by([](const file_entry &e) { return e.file_path.parent_path(); });

        // Hard link count sorting
        inline const auto sort_by_links = sort_by(proj::hard_link_count);
        inline const auto sort_by_links_desc = sort_by_desc(proj::hard_link_count);

        // UID/GID sorting
        inline const auto sort_by_uid = sort_by(proj::uid);
        inline const auto sort_by_gid = sort_by(proj::gid);

        // Natural sorting (number-aware)
        inline constexpr natural_sort_adapter sort_natural{};
        inline constexpr natural_sort_adapter sort_natural_desc{true};
    } // namespace entry

} // namespace frappe

#endif // FRAPPE_SORTERS_HPP
