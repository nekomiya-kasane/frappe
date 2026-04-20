#ifndef FRAPPE_TRANSFORMS_HPP
#define FRAPPE_TRANSFORMS_HPP

#include "frappe/entry.hpp"
#include "frappe/projections.hpp"

#include <functional>
#include <ranges>
#include <tuple>
#include <vector>

namespace frappe {

// ============================================================================
// Select adapter - extract multiple attributes as tuple
// ============================================================================

template <typename... Projs> struct select_adapter {
    std::tuple<Projs...> projections;

    template <std::ranges::input_range R> auto operator()(R &&r) const {
        using T = std::ranges::range_value_t<R>;
        using ResultTuple = std::tuple<std::decay_t<std::invoke_result_t<Projs, T>>...>;

        std::vector<ResultTuple> result;
        for (const auto &item : r) {
            result.push_back(apply_projections(item, std::index_sequence_for<Projs...>{}));
        }
        return result;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const select_adapter &s) {
        return s(std::forward<R>(r));
    }

  private:
    template <typename T, std::size_t... Is> auto apply_projections(const T &item, std::index_sequence<Is...>) const {
        return std::make_tuple(std::get<Is>(projections)(item)...);
    }
};

template <typename... Projs> [[nodiscard]] inline auto select(Projs... projs) {
    return select_adapter<Projs...>{std::make_tuple(std::move(projs)...)};
}

// ============================================================================
// Map adapter - transform each element
// ============================================================================

template <typename Func> struct map_adapter {
    Func func;

    template <std::ranges::input_range R> auto operator()(R &&r) const {
        using T = std::ranges::range_value_t<R>;
        using ResultType = std::decay_t<std::invoke_result_t<Func, T>>;

        std::vector<ResultType> result;
        for (auto &&item : r) {
            result.push_back(func(std::forward<decltype(item)>(item)));
        }
        return result;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const map_adapter &m) {
        return m(std::forward<R>(r));
    }
};

template <typename Func> [[nodiscard]] inline auto map_to(Func func) {
    return map_adapter<Func>{std::move(func)};
}

// ============================================================================
// Flat map adapter - transform and flatten
// ============================================================================

template <typename Func> struct flat_map_adapter {
    Func func;

    template <std::ranges::input_range R> auto operator()(R &&r) const {
        using T = std::ranges::range_value_t<R>;
        using InnerRange = std::decay_t<std::invoke_result_t<Func, T>>;
        using ResultType = std::ranges::range_value_t<InnerRange>;

        std::vector<ResultType> result;
        for (auto &&item : r) {
            auto inner = func(std::forward<decltype(item)>(item));
            for (auto &&inner_item : inner) {
                result.push_back(std::forward<decltype(inner_item)>(inner_item));
            }
        }
        return result;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const flat_map_adapter &fm) {
        return fm(std::forward<R>(r));
    }
};

template <typename Func> [[nodiscard]] inline auto flat_map(Func func) {
    return flat_map_adapter<Func>{std::move(func)};
}

// ============================================================================
// Filter map adapter - filter and transform in one pass
// ============================================================================

template <typename Pred, typename Func> struct filter_map_adapter {
    Pred predicate;
    Func func;

    template <std::ranges::input_range R> auto operator()(R &&r) const {
        using T = std::ranges::range_value_t<R>;
        using ResultType = std::decay_t<std::invoke_result_t<Func, T>>;

        std::vector<ResultType> result;
        for (auto &&item : r) {
            if (predicate(item)) {
                result.push_back(func(std::forward<decltype(item)>(item)));
            }
        }
        return result;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const filter_map_adapter &fm) {
        return fm(std::forward<R>(r));
    }
};

template <typename Pred, typename Func> [[nodiscard]] inline auto filter_map(Pred pred, Func func) {
    return filter_map_adapter<Pred, Func>{std::move(pred), std::move(func)};
}

// ============================================================================
// Zip adapter - combine with another range
// ============================================================================

template <typename Range2> struct zip_adapter {
    Range2 other;

    template <std::ranges::input_range R> auto operator()(R &&r) const {
        using T1 = std::ranges::range_value_t<R>;
        using T2 = std::ranges::range_value_t<Range2>;

        std::vector<std::pair<T1, T2>> result;
        auto it1 = std::ranges::begin(r);
        auto end1 = std::ranges::end(r);
        auto it2 = std::ranges::begin(other);
        auto end2 = std::ranges::end(other);

        while (it1 != end1 && it2 != end2) {
            result.emplace_back(*it1, *it2);
            ++it1;
            ++it2;
        }
        return result;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const zip_adapter &z) {
        return z(std::forward<R>(r));
    }
};

template <typename Range2> [[nodiscard]] inline auto zip_with(Range2 &&other) {
    return zip_adapter<std::decay_t<Range2>>{std::forward<Range2>(other)};
}

// ============================================================================
// Tap adapter - side effect without modifying the stream
// ============================================================================

template <typename Func> struct tap_adapter {
    Func func;

    template <std::ranges::input_range R> auto operator()(R &&r) const {
        std::vector<std::ranges::range_value_t<R>> result;
        for (auto &&item : r) {
            func(item); // Side effect
            result.push_back(std::forward<decltype(item)>(item));
        }
        return result;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const tap_adapter &t) {
        return t(std::forward<R>(r));
    }
};

template <typename Func> [[nodiscard]] inline auto tap(Func func) {
    return tap_adapter<Func>{std::move(func)};
}

// ============================================================================
// Scan adapter - running accumulation
// ============================================================================

template <typename Func, typename Init> struct scan_adapter {
    Func func;
    Init init;

    template <std::ranges::input_range R> auto operator()(R &&r) const {
        using T = std::ranges::range_value_t<R>;
        using AccType = std::decay_t<Init>;

        std::vector<AccType> result;
        AccType acc = init;

        for (const auto &item : r) {
            acc = func(acc, item);
            result.push_back(acc);
        }
        return result;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const scan_adapter &s) {
        return s(std::forward<R>(r));
    }
};

template <typename Func, typename Init> [[nodiscard]] inline auto scan(Func func, Init init) {
    return scan_adapter<Func, Init>{std::move(func), std::move(init)};
}

// ============================================================================
// Reduce adapter - fold to single value
// ============================================================================

template <typename Func, typename Init> struct reduce_adapter {
    Func func;
    Init init;

    template <std::ranges::input_range R> auto operator()(R &&r) const {
        auto acc = init;
        for (const auto &item : r) {
            acc = func(acc, item);
        }
        return acc;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const reduce_adapter &red) {
        return red(std::forward<R>(r));
    }
};

template <typename Func, typename Init> [[nodiscard]] inline auto reduce(Func func, Init init) {
    return reduce_adapter<Func, Init>{std::move(func), std::move(init)};
}

// ============================================================================
// To vector adapter - explicit materialization
// ============================================================================

struct to_vector_adapter {
    template <std::ranges::input_range R> auto operator()(R &&r) const {
        std::vector<std::ranges::range_value_t<R>> result;
        for (auto &&item : r) {
            result.push_back(std::forward<decltype(item)>(item));
        }
        return result;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const to_vector_adapter &tv) {
        return tv(std::forward<R>(r));
    }
};

inline constexpr to_vector_adapter to_vector{};

// ============================================================================
// To set adapter - convert to set (removes duplicates)
// ============================================================================

template <typename Proj = std::identity> struct to_set_adapter {
    Proj projection;

    template <std::ranges::input_range R> auto operator()(R &&r) const {
        using T = std::ranges::range_value_t<R>;
        using Key = std::decay_t<std::invoke_result_t<Proj, T>>;

        std::set<Key> result;
        for (const auto &item : r) {
            result.insert(projection(item));
        }
        return result;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const to_set_adapter &ts) {
        return ts(std::forward<R>(r));
    }
};

inline constexpr to_set_adapter<std::identity> to_set{};

template <typename Proj> [[nodiscard]] inline auto to_set_by(Proj proj) {
    return to_set_adapter<Proj>{std::move(proj)};
}

// ============================================================================
// To map adapter - convert to map
// ============================================================================

template <typename KeyProj, typename ValueProj> struct to_map_adapter {
    KeyProj key_proj;
    ValueProj value_proj;

    template <std::ranges::input_range R> auto operator()(R &&r) const {
        using T = std::ranges::range_value_t<R>;
        using Key = std::decay_t<std::invoke_result_t<KeyProj, T>>;
        using Value = std::decay_t<std::invoke_result_t<ValueProj, T>>;

        std::map<Key, Value> result;
        for (const auto &item : r) {
            result.emplace(key_proj(item), value_proj(item));
        }
        return result;
    }

    template <std::ranges::input_range R> friend auto operator|(R &&r, const to_map_adapter &tm) {
        return tm(std::forward<R>(r));
    }
};

template <typename KeyProj, typename ValueProj>
[[nodiscard]] inline auto to_map(KeyProj key_proj, ValueProj value_proj) {
    return to_map_adapter<KeyProj, ValueProj>{std::move(key_proj), std::move(value_proj)};
}

// ============================================================================
// Common transforms for file entries
// ============================================================================

// Extract paths only
inline const auto paths_only = map_to([](const file_entry &e) { return e.file_path; });

// Extract names only
inline const auto names_only = map_to([](const file_entry &e) { return e.name; });

// Extract sizes only
inline const auto sizes_only = map_to([](const file_entry &e) { return e.size; });

// Name to path map
inline const auto name_to_path = to_map(proj::name, proj::file_path);

// Path to size map
inline const auto path_to_size = to_map(proj::file_path, proj::size);

} // namespace frappe

#endif // FRAPPE_TRANSFORMS_HPP
