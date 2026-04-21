#pragma once

#include "concepts.hpp"
#include "pipeable.hpp"

#include <map>
#include <optional>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/chunk.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/slice.hpp>
#include <range/v3/view/take.hpp>
#include <set>
#include <vector>

namespace rpp {

    // ============================================================================
    // Selection adapters
    // ============================================================================

    inline constexpr struct first_fn : pipeable_base<first_fn> {
        template <input_range R> constexpr auto operator()(R &&r) const -> std::optional<range_value_t<R>> {
            auto it = ::ranges::begin(r);
            if (it != ::ranges::end(r)) {
                return *it;
            }
            return std::nullopt;
        }
    } first{};

    inline constexpr struct last_fn : pipeable_base<last_fn> {
        template <input_range R> constexpr auto operator()(R &&r) const -> std::optional<range_value_t<R>> {
            std::optional<range_value_t<R>> result;
            for (auto &&item : r) {
                result = std::forward<decltype(item)>(item);
            }
            return result;
        }
    } last{};

    inline constexpr struct take_fn {
        [[nodiscard]] constexpr auto operator()(std::size_t n) const {
            return make_closure([n](auto &&r) {
                return std::forward<decltype(r)>(r) | ::ranges::views::take(static_cast<std::ptrdiff_t>(n));
            });
        }
    } take{};

    inline constexpr struct drop_fn {
        [[nodiscard]] constexpr auto operator()(std::size_t n) const {
            return make_closure([n](auto &&r) {
                return std::forward<decltype(r)>(r) | ::ranges::views::drop(static_cast<std::ptrdiff_t>(n));
            });
        }
    } drop{};

    inline constexpr auto &skip = drop;

    inline constexpr struct slice_fn {
        [[nodiscard]] constexpr auto operator()(std::size_t start, std::size_t count) const {
            return make_closure([start, count](auto &&r) {
                return std::forward<decltype(r)>(r) |
                       ::ranges::views::slice(static_cast<std::ptrdiff_t>(start),
                                              static_cast<std::ptrdiff_t>(start + count));
            });
        }
    } slice{};

    inline constexpr struct nth_fn {
        [[nodiscard]] constexpr auto operator()(std::size_t n) const {
            return make_closure([n](auto &&r) -> std::optional<range_value_t<decltype(r)>> {
                std::size_t i = 0;
                for (auto &&item : r) {
                    if (i++ == n) {
                        return std::forward<decltype(item)>(item);
                    }
                }
                return std::nullopt;
            });
        }
    } nth{};

    // ============================================================================
    // Statistical adapters
    // ============================================================================

    inline constexpr struct count_fn : pipeable_base<count_fn> {
        template <input_range R> constexpr auto operator()(R &&r) const { return ::ranges::distance(r); }
    } count{};

    template <typename Proj = std::identity> struct sum_view : pipeable_base<sum_view<Proj>> {
        Proj proj;

        constexpr explicit sum_view(Proj p = {}) : proj(std::move(p)) {}

        template <input_range R> constexpr auto operator()(R &&r) const {
            using T = std::invoke_result_t<Proj, range_value_t<R>>;
            T result{};
            for (const auto &item : r) {
                result += std::invoke(proj, item);
            }
            return result;
        }
    };

    inline constexpr struct sum_fn {
        [[nodiscard]] constexpr auto operator()() const { return sum_view<>{}; }

        template <typename Proj> [[nodiscard]] constexpr auto operator()(Proj proj) const {
            return sum_view<Proj>{std::move(proj)};
        }
    } sum{};

    template <typename Proj = std::identity> struct product_view : pipeable_base<product_view<Proj>> {
        Proj proj;

        constexpr explicit product_view(Proj p = {}) : proj(std::move(p)) {}

        template <input_range R> constexpr auto operator()(R &&r) const {
            using T = std::invoke_result_t<Proj, range_value_t<R>>;
            T result{1};
            for (const auto &item : r) {
                result *= std::invoke(proj, item);
            }
            return result;
        }
    };

    inline constexpr struct product_fn {
        [[nodiscard]] constexpr auto operator()() const { return product_view<>{}; }

        template <typename Proj> [[nodiscard]] constexpr auto operator()(Proj proj) const {
            return product_view<Proj>{std::move(proj)};
        }
    } product{};

    // ============================================================================
    // Grouping adapters
    // ============================================================================

    inline constexpr struct chunk_fn {
        [[nodiscard]] constexpr auto operator()(std::size_t n) const {
            return make_closure([n](auto &&r) {
                return std::forward<decltype(r)>(r) | ::ranges::views::chunk(static_cast<std::ptrdiff_t>(n));
            });
        }
    } chunk{};

    inline constexpr struct enumerate_fn {
        [[nodiscard]] constexpr auto operator()() const {
            return make_closure([](auto &&r) { return std::forward<decltype(r)>(r) | ::ranges::views::enumerate; });
        }

        [[nodiscard]] constexpr auto operator()(std::size_t start) const {
            return make_closure([start](auto &&r) {
                std::size_t idx = start;
                using T = range_value_t<decltype(r)>;
                std::vector<std::pair<std::size_t, T>> result;
                for (auto &&item : r) {
                    result.emplace_back(idx++, std::forward<decltype(item)>(item));
                }
                return result;
            });
        }
    } enumerate{};

    template <typename Proj> struct group_by_view : pipeable_base<group_by_view<Proj>> {
        Proj proj;

        constexpr explicit group_by_view(Proj p) : proj(std::move(p)) {}

        template <input_range R> constexpr auto operator()(R &&r) const {
            using Key = std::decay_t<std::invoke_result_t<Proj, range_value_t<R>>>;
            using Value = range_value_t<R>;

            std::map<Key, std::vector<Value>> groups;
            for (auto &&item : r) {
                groups[std::invoke(proj, item)].push_back(std::forward<decltype(item)>(item));
            }
            return groups;
        }
    };

    inline constexpr struct group_by_fn {
        template <typename Proj> [[nodiscard]] constexpr auto operator()(Proj proj) const {
            return group_by_view<Proj>{std::move(proj)};
        }
    } group_by{};

    template <typename Proj> struct count_by_view : pipeable_base<count_by_view<Proj>> {
        Proj proj;

        constexpr explicit count_by_view(Proj p) : proj(std::move(p)) {}

        template <input_range R> constexpr auto operator()(R &&r) const {
            using Key = std::decay_t<std::invoke_result_t<Proj, range_value_t<R>>>;

            std::map<Key, std::size_t> counts;
            for (const auto &item : r) {
                ++counts[std::invoke(proj, item)];
            }
            return counts;
        }
    };

    inline constexpr struct count_by_fn {
        template <typename Proj> [[nodiscard]] constexpr auto operator()(Proj proj) const {
            return count_by_view<Proj>{std::move(proj)};
        }
    } count_by{};

    // ============================================================================
    // Collection adapters
    // ============================================================================

    inline constexpr struct to_vector_fn : pipeable_base<to_vector_fn> {
        template <input_range R> constexpr auto operator()(R &&r) const {
            return std::forward<R>(r) | ::ranges::to<std::vector>();
        }
    } to_vector{};

    inline constexpr struct to_set_fn : pipeable_base<to_set_fn> {
        template <input_range R> constexpr auto operator()(R &&r) const {
            return std::forward<R>(r) | ::ranges::to<std::set>();
        }
    } to_set{};

    template <typename KeyProj, typename ValueProj = std::identity>
    struct to_map_view : pipeable_base<to_map_view<KeyProj, ValueProj>> {
        KeyProj key_proj;
        ValueProj value_proj;

        constexpr to_map_view(KeyProj kp, ValueProj vp = {}) : key_proj(std::move(kp)), value_proj(std::move(vp)) {}

        template <input_range R> constexpr auto operator()(R &&r) const {
            using K = std::decay_t<std::invoke_result_t<KeyProj, range_value_t<R>>>;
            using V = std::decay_t<std::invoke_result_t<ValueProj, range_value_t<R>>>;

            std::map<K, V> result;
            for (auto &&item : r) {
                result.emplace(std::invoke(key_proj, item), std::invoke(value_proj, item));
            }
            return result;
        }
    };

    inline constexpr struct to_map_fn {
        template <typename KeyProj, typename ValueProj = std::identity>
        [[nodiscard]] constexpr auto operator()(KeyProj key_proj, ValueProj value_proj = {}) const {
            return to_map_view<KeyProj, ValueProj>{std::move(key_proj), std::move(value_proj)};
        }
    } to_map{};

    // ============================================================================
    // Fold/Reduce
    // ============================================================================

    template <typename BinaryOp, typename Init> struct fold_view : pipeable_base<fold_view<BinaryOp, Init>> {
        BinaryOp op;
        Init init;

        constexpr fold_view(BinaryOp o, Init i) : op(std::move(o)), init(std::move(i)) {}

        template <input_range R> constexpr auto operator()(R &&r) const {
            auto result = init;
            for (auto &&item : r) {
                result = op(std::move(result), std::forward<decltype(item)>(item));
            }
            return result;
        }
    };

    inline constexpr struct fold_fn {
        template <typename BinaryOp, typename Init>
        [[nodiscard]] constexpr auto operator()(Init init, BinaryOp op) const {
            return fold_view<BinaryOp, Init>{std::move(op), std::move(init)};
        }
    } fold{};

    inline constexpr struct reduce_fn {
        template <typename BinaryOp> [[nodiscard]] constexpr auto operator()(BinaryOp op) const {
            return make_closure([op = std::move(op)](auto &&r) -> std::optional<range_value_t<decltype(r)>> {
                auto it = ::ranges::begin(r);
                auto end = ::ranges::end(r);
                if (it == end) {
                    return std::nullopt;
                }

                auto result = *it++;
                while (it != end) {
                    result = op(std::move(result), *it++);
                }
                return result;
            });
        }
    } reduce{};

} // namespace rpp
