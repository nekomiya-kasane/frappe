#pragma once

#include "concepts.hpp"
#include "pipeable.hpp"

#include <iomanip>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/take.hpp>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace rpp {

// ============================================================================
// materialize() - eagerly collect a lazy range into owning_view<vector<T>>
// ============================================================================

inline constexpr struct materialize_fn : pipeable_base<materialize_fn> {
    template <input_range R> constexpr auto operator()(R &&r) const {
        using T = range_value_t<R>;
        auto vec = std::forward<R>(r) | ::ranges::to<std::vector<T>>();
        return owning_view<std::vector<T>>{std::move(vec)};
    }
} materialize{};

// ============================================================================
// from(container) - unified entry point, converts container to a pipeable range
// ============================================================================

template <typename Container> [[nodiscard]] constexpr auto from(Container &&c) {
    if constexpr (std::is_lvalue_reference_v<Container>) {
        return ::ranges::views::all(c);
    } else {
        return owning_view<std::decay_t<Container>>{std::forward<Container>(c)};
    }
}

// ============================================================================
// for_each(fn) - terminal consumption adapter
// ============================================================================

template <typename Fn> struct for_each_view : pipeable_base<for_each_view<Fn>> {
    Fn fn;

    constexpr explicit for_each_view(Fn f) : fn(std::move(f)) {}

    template <input_range R> constexpr void operator()(R &&r) const {
        for (auto &&item : r) {
            fn(std::forward<decltype(item)>(item));
        }
    }
};

inline constexpr struct for_each_fn {
    template <typename Fn> [[nodiscard]] constexpr auto operator()(Fn fn) const {
        return for_each_view<Fn>{std::move(fn)};
    }
} for_each{};

// ============================================================================
// join_str(sep) - string concatenation terminal
// ============================================================================

struct join_str_view : pipeable_base<join_str_view> {
    std::string separator;

    explicit join_str_view(std::string sep) : separator(std::move(sep)) {}

    template <input_range R> auto operator()(R &&r) const -> std::string {
        std::ostringstream oss;
        bool first = true;
        for (auto &&item : r) {
            if (!first) {
                oss << separator;
            }
            oss << std::forward<decltype(item)>(item);
            first = false;
        }
        return oss.str();
    }
};

inline constexpr struct join_str_fn {
    [[nodiscard]] auto operator()(std::string sep = ", ") const { return join_str_view{std::move(sep)}; }
} join_str{};

// ============================================================================
// to<Container>() - generic collection terminal
// ============================================================================

template <template <typename...> class Container>
struct to_container_view : pipeable_base<to_container_view<Container>> {
    template <input_range R> constexpr auto operator()(R &&r) const {
        using T = range_value_t<R>;
        Container<T> result;
        for (auto &&item : r) {
            if constexpr (requires { result.push_back(std::forward<decltype(item)>(item)); }) {
                result.push_back(std::forward<decltype(item)>(item));
            } else if constexpr (requires { result.insert(std::forward<decltype(item)>(item)); }) {
                result.insert(std::forward<decltype(item)>(item));
            } else if constexpr (requires { result.push_front(std::forward<decltype(item)>(item)); }) {
                result.push_front(std::forward<decltype(item)>(item));
            }
        }
        return result;
    }
};

template <template <typename...> class Container> [[nodiscard]] constexpr auto to() {
    return to_container_view<Container>{};
}

// ============================================================================
// to_unordered_map(key_proj, value_proj) - collect into unordered_map
// ============================================================================

template <typename KeyProj, typename ValueProj = std::identity>
struct to_unordered_map_view : pipeable_base<to_unordered_map_view<KeyProj, ValueProj>> {
    KeyProj key_proj;
    ValueProj value_proj;

    constexpr to_unordered_map_view(KeyProj kp, ValueProj vp = {})
        : key_proj(std::move(kp)), value_proj(std::move(vp)) {}

    template <input_range R> constexpr auto operator()(R &&r) const {
        using K = std::decay_t<std::invoke_result_t<KeyProj, range_value_t<R>>>;
        using V = std::decay_t<std::invoke_result_t<ValueProj, range_value_t<R>>>;

        std::unordered_map<K, V> result;
        for (auto &&item : r) {
            result.emplace(std::invoke(key_proj, item), std::invoke(value_proj, item));
        }
        return result;
    }
};

inline constexpr struct to_unordered_map_fn {
    template <typename KeyProj, typename ValueProj = std::identity>
    [[nodiscard]] constexpr auto operator()(KeyProj key_proj, ValueProj value_proj = {}) const {
        return to_unordered_map_view<KeyProj, ValueProj>{std::move(key_proj), std::move(value_proj)};
    }
} to_unordered_map{};

// ============================================================================
// format_table() - table formatting terminal
// ============================================================================

template <typename... Headers> struct format_table_view : pipeable_base<format_table_view<Headers...>> {
    std::tuple<Headers...> headers;

    constexpr explicit format_table_view(Headers... hs) : headers(std::move(hs)...) {}

    template <input_range R> auto operator()(R &&r) const -> std::string {
        // Collect all rows as strings
        using T = range_value_t<R>;

        std::vector<std::vector<std::string>> rows;
        std::vector<std::size_t> col_widths;

        // Initialize with header widths
        if constexpr (sizeof...(Headers) > 0) {
            std::apply(
                [&](const auto &...h) {
                    std::vector<std::string> header_row;
                    ((header_row.push_back(std::string(h))), ...);
                    col_widths.resize(header_row.size(), 0);
                    for (std::size_t i = 0; i < header_row.size(); ++i) {
                        col_widths[i] = header_row[i].size();
                    }
                    rows.push_back(std::move(header_row));
                },
                headers);
        }

        // Add data rows
        for (auto &&item : r) {
            std::vector<std::string> row;
            if constexpr (requires { std::tuple_size<T>::value; }) {
                std::apply(
                    [&](const auto &...fields) { ((row.push_back((std::ostringstream{} << fields).str())), ...); },
                    item);
            } else {
                std::ostringstream oss;
                oss << item;
                row.push_back(oss.str());
            }

            if (col_widths.size() < row.size()) {
                col_widths.resize(row.size(), 0);
            }
            for (std::size_t i = 0; i < row.size(); ++i) {
                col_widths[i] = std::max(col_widths[i], row[i].size());
            }
            rows.push_back(std::move(row));
        }

        // Format output
        std::ostringstream out;
        auto write_separator = [&] {
            out << '+';
            for (auto w : col_widths) {
                out << std::string(w + 2, '-') << '+';
            }
            out << '\n';
        };

        if (rows.empty()) {
            return "";
        }

        std::size_t start = 0;
        if constexpr (sizeof...(Headers) > 0) {
            write_separator();
            out << '|';
            for (std::size_t i = 0; i < rows[0].size(); ++i) {
                out << ' ' << std::left << std::setw(static_cast<int>(col_widths[i])) << rows[0][i] << " |";
            }
            out << '\n';
            start = 1;
        }
        write_separator();

        for (std::size_t r = start; r < rows.size(); ++r) {
            out << '|';
            for (std::size_t i = 0; i < rows[r].size(); ++i) {
                out << ' ' << std::left << std::setw(static_cast<int>(col_widths[i])) << rows[r][i] << " |";
            }
            out << '\n';
        }
        write_separator();

        return out.str();
    }
};

inline constexpr struct format_table_fn {
    [[nodiscard]] auto operator()() const { return format_table_view<>{}; }

    template <typename... Headers> [[nodiscard]] auto operator()(Headers... headers) const {
        return format_table_view<Headers...>{std::move(headers)...};
    }
} format_table{};

// ============================================================================
// group_aggregate(key, agg1, agg2, ...) - group-by with multiple aggregations
// ============================================================================

template <typename KeyProj, typename... Aggs>
struct group_aggregate_view : pipeable_base<group_aggregate_view<KeyProj, Aggs...>> {
    KeyProj key_proj;
    std::tuple<Aggs...> aggs;

    constexpr group_aggregate_view(KeyProj kp, Aggs... as) : key_proj(std::move(kp)), aggs(std::move(as)...) {}

    template <input_range R> constexpr auto operator()(R &&r) const {
        using T = range_value_t<R>;
        using Key = std::decay_t<std::invoke_result_t<KeyProj, T>>;

        // Group by key
        std::map<Key, std::vector<T>> groups;
        for (auto &&item : r) {
            groups[std::invoke(key_proj, item)].push_back(std::forward<decltype(item)>(item));
        }

        // Apply aggregates to each group
        using ResultTuple = std::tuple<Key, decltype(std::declval<Aggs>()(std::declval<std::vector<T> &>()))...>;

        std::vector<ResultTuple> result;
        result.reserve(groups.size());

        for (auto &[key, items] : groups) {
            result.push_back(std::apply([&](const auto &...agg) { return std::make_tuple(key, agg(items)...); }, aggs));
        }
        return result;
    }
};

inline constexpr struct group_aggregate_fn {
    template <typename KeyProj, typename... Aggs>
    [[nodiscard]] constexpr auto operator()(KeyProj key_proj, Aggs... aggs) const {
        return group_aggregate_view<KeyProj, Aggs...>{std::move(key_proj), std::move(aggs)...};
    }
} group_aggregate{};

// ============================================================================
// having(pred) - post-filter on grouped/aggregated results
// ============================================================================

template <typename Pred> struct having_view : pipeable_base<having_view<Pred>> {
    Pred pred;

    constexpr explicit having_view(Pred p) : pred(std::move(p)) {}

    template <input_range R> constexpr auto operator()(R &&r) const {
        using T = range_value_t<R>;
        std::vector<T> result;
        for (auto &&item : r) {
            if (pred(item)) {
                result.push_back(std::forward<decltype(item)>(item));
            }
        }
        return result;
    }
};

inline constexpr struct having_fn {
    template <typename Pred> [[nodiscard]] constexpr auto operator()(Pred pred) const {
        return having_view<Pred>{std::move(pred)};
    }
} having{};

// ============================================================================
// sliding_window(n) - overlapping sliding window
// ============================================================================

template <typename T = void> struct sliding_window_view : pipeable_base<sliding_window_view<T>> {
    std::size_t window_size;

    constexpr explicit sliding_window_view(std::size_t n) : window_size(n) {}

    template <input_range R> constexpr auto operator()(R &&r) const {
        using V = range_value_t<R>;
        auto items = std::forward<R>(r) | ::ranges::to<std::vector<V>>();

        std::vector<std::vector<V>> result;
        if (items.size() >= window_size) {
            result.reserve(items.size() - window_size + 1);
            for (std::size_t i = 0; i + window_size <= items.size(); ++i) {
                result.emplace_back(items.begin() + i, items.begin() + i + window_size);
            }
        }
        return result;
    }
};

inline constexpr struct sliding_window_fn {
    [[nodiscard]] constexpr auto operator()(std::size_t n) const { return sliding_window_view<>{n}; }
} sliding_window{};

// ============================================================================
// zip_with(r2, fn) - zip two ranges and transform in one step
// ============================================================================

template <typename R2, typename Fn> struct zip_with_view : pipeable_base<zip_with_view<R2, Fn>> {
    R2 other;
    Fn fn;

    constexpr zip_with_view(R2 o, Fn f) : other(std::move(o)), fn(std::move(f)) {}

    template <input_range R1> constexpr auto operator()(R1 &&r1) const {
        auto v1 = std::forward<R1>(r1) | ::ranges::to<std::vector>();
        auto v2 = other | ::ranges::to<std::vector>();

        using Result = std::invoke_result_t<Fn, range_value_t<R1>, range_value_t<R2>>;
        std::vector<Result> result;
        auto n = std::min(v1.size(), v2.size());
        result.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            result.push_back(std::invoke(fn, v1[i], v2[i]));
        }
        return result;
    }
};

inline constexpr struct zip_with_fn {
    template <input_range R2, typename Fn> [[nodiscard]] constexpr auto operator()(R2 &&other, Fn fn) const {
        return zip_with_view<std::decay_t<R2>, Fn>{std::forward<R2>(other) | ::ranges::to<std::vector>(),
                                                   std::move(fn)};
    }
} zip_with{};

// ============================================================================
// enumerate_map(fn) - index-aware transform: fn(index, element) -> result
// ============================================================================

template <typename Fn> struct enumerate_map_view : pipeable_base<enumerate_map_view<Fn>> {
    Fn fn;

    constexpr explicit enumerate_map_view(Fn f) : fn(std::move(f)) {}

    template <input_range R> constexpr auto operator()(R &&r) const {
        using T = range_value_t<R>;
        using Result = std::invoke_result_t<Fn, std::size_t, const T &>;
        std::vector<Result> result;
        std::size_t idx = 0;
        for (auto &&item : r) {
            result.push_back(std::invoke(fn, idx++, item));
        }
        return result;
    }
};

inline constexpr struct enumerate_map_fn {
    template <typename Fn> [[nodiscard]] constexpr auto operator()(Fn fn) const {
        return enumerate_map_view<Fn>{std::move(fn)};
    }
} enumerate_map{};

// ============================================================================
// batch(n, fn) - chunk into groups of n and aggregate each chunk
// ============================================================================

template <typename Fn> struct batch_view : pipeable_base<batch_view<Fn>> {
    std::size_t batch_size;
    Fn fn;

    constexpr batch_view(std::size_t n, Fn f) : batch_size(n), fn(std::move(f)) {}

    template <input_range R> constexpr auto operator()(R &&r) const {
        using T = range_value_t<R>;
        auto items = std::forward<R>(r) | ::ranges::to<std::vector<T>>();

        using ChunkRange = std::vector<T>;
        using Result = std::invoke_result_t<Fn, const ChunkRange &>;
        std::vector<Result> result;

        for (std::size_t i = 0; i < items.size(); i += batch_size) {
            auto end = std::min(i + batch_size, items.size());
            ChunkRange chunk(items.begin() + i, items.begin() + end);
            result.push_back(std::invoke(fn, chunk));
        }
        return result;
    }
};

inline constexpr struct batch_fn {
    template <typename Fn> [[nodiscard]] constexpr auto operator()(std::size_t n, Fn fn) const {
        return batch_view<Fn>{n, std::move(fn)};
    }
} batch{};

// ============================================================================
// pairwise() - adjacent element pairs: [a,b,c,d] -> [(a,b),(b,c),(c,d)]
// ============================================================================

inline constexpr struct pairwise_fn : pipeable_base<pairwise_fn> {
    template <input_range R> constexpr auto operator()(R &&r) const {
        using T = range_value_t<R>;
        auto items = std::forward<R>(r) | ::ranges::to<std::vector<T>>();

        std::vector<std::pair<T, T>> result;
        if (items.size() >= 2) {
            result.reserve(items.size() - 1);
            for (std::size_t i = 0; i + 1 < items.size(); ++i) {
                result.emplace_back(items[i], items[i + 1]);
            }
        }
        return result;
    }
} pairwise{};

// ============================================================================
// stride(n) - take every nth element
// ============================================================================

struct stride_view : pipeable_base<stride_view> {
    std::size_t step;

    constexpr explicit stride_view(std::size_t n) : step(n) {}

    template <input_range R> constexpr auto operator()(R &&r) const {
        using T = range_value_t<R>;
        std::vector<T> result;
        std::size_t idx = 0;
        for (auto &&item : r) {
            if (idx % step == 0) {
                result.push_back(std::forward<decltype(item)>(item));
            }
            ++idx;
        }
        return owning_view<std::vector<T>>{std::move(result)};
    }
};

inline constexpr struct stride_fn {
    [[nodiscard]] constexpr auto operator()(std::size_t n) const { return stride_view{n}; }
} stride{};

// ============================================================================
// take_while(pred) - take elements while predicate holds
// ============================================================================

template <typename Pred> struct take_while_view : pipeable_base<take_while_view<Pred>> {
    Pred pred;

    constexpr explicit take_while_view(Pred p) : pred(std::move(p)) {}

    template <input_range R> constexpr auto operator()(R &&r) const {
        using T = range_value_t<R>;
        std::vector<T> result;
        for (auto &&item : r) {
            if (!pred(item)) {
                break;
            }
            result.push_back(std::forward<decltype(item)>(item));
        }
        return owning_view<std::vector<T>>{std::move(result)};
    }
};

inline constexpr struct take_while_fn {
    template <typename Pred> [[nodiscard]] constexpr auto operator()(Pred pred) const {
        return take_while_view<Pred>{std::move(pred)};
    }
} take_while{};

// ============================================================================
// drop_while(pred) - drop elements while predicate holds, keep the rest
// ============================================================================

template <typename Pred> struct drop_while_view : pipeable_base<drop_while_view<Pred>> {
    Pred pred;

    constexpr explicit drop_while_view(Pred p) : pred(std::move(p)) {}

    template <input_range R> constexpr auto operator()(R &&r) const {
        using T = range_value_t<R>;
        std::vector<T> result;
        bool dropping = true;
        for (auto &&item : r) {
            if (dropping && pred(item)) {
                continue;
            }
            dropping = false;
            result.push_back(std::forward<decltype(item)>(item));
        }
        return owning_view<std::vector<T>>{std::move(result)};
    }
};

inline constexpr struct drop_while_fn {
    template <typename Pred> [[nodiscard]] constexpr auto operator()(Pred pred) const {
        return drop_while_view<Pred>{std::move(pred)};
    }
} drop_while{};

// ============================================================================
// generate(fn, n) - create a range by calling fn() n times
// ============================================================================

template <typename Fn> [[nodiscard]] auto generate(Fn fn, std::size_t n) {
    using T = std::invoke_result_t<Fn>;
    std::vector<T> result;
    result.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        result.push_back(fn());
    }
    return owning_view<std::vector<T>>{std::move(result)};
}

// ============================================================================
// generate_n(fn, n) - create a range by calling fn(index) n times
// ============================================================================

template <typename Fn> [[nodiscard]] auto generate_n(Fn fn, std::size_t n) {
    using T = std::invoke_result_t<Fn, std::size_t>;
    std::vector<T> result;
    result.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        result.push_back(fn(i));
    }
    return owning_view<std::vector<T>>{std::move(result)};
}

// ============================================================================
// iota(start, end) / iota(end) - integer range factory
// ============================================================================

template <typename T = int> [[nodiscard]] auto iota(T start, T end_val) {
    std::vector<T> result;
    if (start < end_val) {
        result.reserve(static_cast<std::size_t>(end_val - start));
        for (T i = start; i < end_val; ++i) {
            result.push_back(i);
        }
    }
    return owning_view<std::vector<T>>{std::move(result)};
}

template <typename T = int> [[nodiscard]] auto iota(T end_val) {
    return iota(T{0}, end_val);
}

// ============================================================================
// cache() - eagerly evaluate and memoize a lazy range for multi-pass iteration
// ============================================================================

inline constexpr struct cache_fn : pipeable_base<cache_fn> {
    template <input_range R> constexpr auto operator()(R &&r) const {
        using T = range_value_t<R>;
        auto vec = std::forward<R>(r) | ::ranges::to<std::vector<T>>();
        return owning_view<std::vector<T>>{std::move(vec)};
    }
} cache{};

} // namespace rpp
