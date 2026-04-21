#pragma once

#include "concepts.hpp"
#include "pipeable.hpp"

#include <optional>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/zip.hpp>
#include <unordered_map>
#include <vector>

namespace rpp {

    // ============================================================================
    // Inner join (INNER JOIN)
    // ============================================================================

    template <typename R2, typename Key1, typename Key2, typename Combine>
    struct join_view : pipeable_base<join_view<R2, Key1, Key2, Combine>> {
        R2 other;
        Key1 key1;
        Key2 key2;
        Combine combine;

        constexpr join_view(R2 o, Key1 k1, Key2 k2, Combine c)
            : other(std::move(o)), key1(std::move(k1)), key2(std::move(k2)), combine(std::move(c)) {}

        template <input_range R1> constexpr auto operator()(R1 &&r1) const {
            using T1 = range_value_t<R1>;
            using T2 = range_value_t<R2>;
            using K = std::decay_t<std::invoke_result_t<Key1, T1>>;
            using Result = std::invoke_result_t<Combine, T1, T2>;

            // Build hash index on right table
            std::unordered_multimap<K, T2> index;
            for (const auto &item : other) {
                index.emplace(std::invoke(key2, item), item);
            }

            // Perform join
            std::vector<Result> result;
            for (const auto &left : r1) {
                auto k = std::invoke(key1, left);
                auto [begin, end] = index.equal_range(k);
                for (auto it = begin; it != end; ++it) {
                    result.push_back(combine(left, it->second));
                }
            }
            return result;
        }
    };

    inline constexpr struct join_fn {
        template <input_range R2, typename Key1, typename Key2>
        [[nodiscard]] constexpr auto operator()(R2 &&other, Key1 key1, Key2 key2) const {
            return (*this)(std::forward<R2>(other), std::move(key1), std::move(key2),
                           [](const auto &a, const auto &b) { return std::make_pair(a, b); });
        }

        template <input_range R2, typename Key1, typename Key2, typename Combine>
        [[nodiscard]] constexpr auto operator()(R2 &&other, Key1 key1, Key2 key2, Combine combine) const {
            using R2Decayed = std::decay_t<R2>;
            return join_view<R2Decayed, Key1, Key2, Combine>{std::forward<R2>(other) | ::ranges::to<std::vector>(),
                                                             std::move(key1), std::move(key2), std::move(combine)};
        }
    } join{};

    // ============================================================================
    // Left join (LEFT JOIN) - right side is optional
    // ============================================================================

    template <typename R2, typename Key1, typename Key2, typename Combine>
    struct left_join_view : pipeable_base<left_join_view<R2, Key1, Key2, Combine>> {
        R2 other;
        Key1 key1;
        Key2 key2;
        Combine combine;

        constexpr left_join_view(R2 o, Key1 k1, Key2 k2, Combine c)
            : other(std::move(o)), key1(std::move(k1)), key2(std::move(k2)), combine(std::move(c)) {}

        template <input_range R1> constexpr auto operator()(R1 &&r1) const {
            using T1 = range_value_t<R1>;
            using T2 = range_value_t<R2>;
            using K = std::decay_t<std::invoke_result_t<Key1, T1>>;
            using Result = std::invoke_result_t<Combine, T1, std::optional<T2>>;

            // Build hash index on right table
            std::unordered_multimap<K, T2> index;
            for (const auto &item : other) {
                index.emplace(std::invoke(key2, item), item);
            }

            // Perform left join
            std::vector<Result> result;
            for (const auto &left : r1) {
                auto k = std::invoke(key1, left);
                auto [begin, end] = index.equal_range(k);

                if (begin == end) {
                    // No match - include left with nullopt
                    result.push_back(combine(left, std::nullopt));
                } else {
                    for (auto it = begin; it != end; ++it) {
                        result.push_back(combine(left, std::optional<T2>(it->second)));
                    }
                }
            }
            return result;
        }
    };

    inline constexpr struct left_join_fn {
        template <input_range R2, typename Key1, typename Key2>
        [[nodiscard]] constexpr auto operator()(R2 &&other, Key1 key1, Key2 key2) const {
            return (*this)(std::forward<R2>(other), std::move(key1), std::move(key2),
                           [](const auto &a, const auto &b) { return std::make_pair(a, b); });
        }

        template <input_range R2, typename Key1, typename Key2, typename Combine>
        [[nodiscard]] constexpr auto operator()(R2 &&other, Key1 key1, Key2 key2, Combine combine) const {
            using R2Decayed = std::decay_t<R2>;
            return left_join_view<R2Decayed, Key1, Key2, Combine>{std::forward<R2>(other) | ::ranges::to<std::vector>(),
                                                                  std::move(key1), std::move(key2), std::move(combine)};
        }
    } left_join{};

    // ============================================================================
    // Right join (RIGHT JOIN)
    // ============================================================================

    template <typename R2, typename Key1, typename Key2, typename Combine>
    struct right_join_view : pipeable_base<right_join_view<R2, Key1, Key2, Combine>> {
        R2 other;
        Key1 key1;
        Key2 key2;
        Combine combine;

        constexpr right_join_view(R2 o, Key1 k1, Key2 k2, Combine c)
            : other(std::move(o)), key1(std::move(k1)), key2(std::move(k2)), combine(std::move(c)) {}

        template <input_range R1> constexpr auto operator()(R1 &&r1) const {
            using T1 = range_value_t<R1>;
            using T2 = range_value_t<R2>;
            using K = std::decay_t<std::invoke_result_t<Key1, T1>>;
            using Result = std::invoke_result_t<Combine, std::optional<T1>, T2>;

            // Build hash index on left table
            std::unordered_multimap<K, T1> index;
            for (const auto &item : r1) {
                index.emplace(std::invoke(key1, item), item);
            }

            // Perform right join
            std::vector<Result> result;
            for (const auto &right : other) {
                auto k = std::invoke(key2, right);
                auto [begin, end] = index.equal_range(k);

                if (begin == end) {
                    result.push_back(combine(std::nullopt, right));
                } else {
                    for (auto it = begin; it != end; ++it) {
                        result.push_back(combine(std::optional<T1>(it->second), right));
                    }
                }
            }
            return result;
        }
    };

    inline constexpr struct right_join_fn {
        template <input_range R2, typename Key1, typename Key2>
        [[nodiscard]] constexpr auto operator()(R2 &&other, Key1 key1, Key2 key2) const {
            return (*this)(std::forward<R2>(other), std::move(key1), std::move(key2),
                           [](const auto &a, const auto &b) { return std::make_pair(a, b); });
        }

        template <input_range R2, typename Key1, typename Key2, typename Combine>
        [[nodiscard]] constexpr auto operator()(R2 &&other, Key1 key1, Key2 key2, Combine combine) const {
            using R2Decayed = std::decay_t<R2>;
            return right_join_view<R2Decayed, Key1, Key2, Combine>{
                std::forward<R2>(other) | ::ranges::to<std::vector>(), std::move(key1), std::move(key2),
                std::move(combine)};
        }
    } right_join{};

    // ============================================================================
    // Cartesian product (CROSS JOIN)
    // ============================================================================

    template <typename R2, typename Combine> struct cross_join_view : pipeable_base<cross_join_view<R2, Combine>> {
        R2 other;
        Combine combine;

        constexpr cross_join_view(R2 o, Combine c) : other(std::move(o)), combine(std::move(c)) {}

        template <input_range R1> constexpr auto operator()(R1 &&r1) const {
            using T1 = range_value_t<R1>;
            using T2 = range_value_t<R2>;
            using Result = std::invoke_result_t<Combine, T1, T2>;

            std::vector<Result> result;
            for (const auto &a : r1) {
                for (const auto &b : other) {
                    result.push_back(combine(a, b));
                }
            }
            return result;
        }
    };

    inline constexpr struct cross_join_fn {
        template <input_range R2> [[nodiscard]] constexpr auto operator()(R2 &&other) const {
            return (*this)(std::forward<R2>(other), [](const auto &a, const auto &b) { return std::make_pair(a, b); });
        }

        template <input_range R2, typename Combine>
        [[nodiscard]] constexpr auto operator()(R2 &&other, Combine combine) const {
            using R2Decayed = std::decay_t<R2>;
            return cross_join_view<R2Decayed, Combine>{std::forward<R2>(other) | ::ranges::to<std::vector>(),
                                                       std::move(combine)};
        }
    } cross_join{};

    // ============================================================================
    // Zip (parallel join) - lazy version using range-v3 zip
    // ============================================================================

    inline constexpr struct zip_fn {
        template <input_range R2> [[nodiscard]] constexpr auto operator()(R2 &&other) const {
            return make_closure([o = std::forward<R2>(other) | ::ranges::to<std::vector>()](auto &&r1) {
                return ::ranges::views::zip(std::forward<decltype(r1)>(r1), o);
            });
        }

        template <input_range R2, typename Combine>
        [[nodiscard]] constexpr auto operator()(R2 &&other, Combine combine) const {
            return make_closure(
                [o = std::forward<R2>(other) | ::ranges::to<std::vector>(), c = std::move(combine)](auto &&r1) {
                    return ::ranges::views::zip(std::forward<decltype(r1)>(r1), o) |
                           ::ranges::views::transform([c](auto &&pair) {
                               return c(std::get<0>(std::forward<decltype(pair)>(pair)),
                                        std::get<1>(std::forward<decltype(pair)>(pair)));
                           });
                });
        }
    } zip{};

} // namespace rpp
