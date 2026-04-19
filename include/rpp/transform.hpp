#pragma once

#include "concepts.hpp"
#include "pipeable.hpp"
#include <range/v3/view/transform.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/reverse.hpp>
#include <range/v3/view/cycle.hpp>
#include <range/v3/view/repeat_n.hpp>
#include <range/v3/view/intersperse.hpp>
#include <range/v3/range/conversion.hpp>
#include <vector>
#include <random>
#include <algorithm>

namespace rpp {

// ============================================================================
// Map/Transform
// ============================================================================

template<typename Fn>
struct map_view : pipeable_base<map_view<Fn>> {
    Fn fn;
    
    constexpr explicit map_view(Fn f) : fn(std::move(f)) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        return std::forward<R>(r) | ::ranges::views::transform(fn);
    }
};

template<typename Fn>
map_view(Fn) -> map_view<Fn>;

inline constexpr struct map_fn {
    template<typename Fn>
    [[nodiscard]] constexpr auto operator()(Fn fn) const {
        return map_view<Fn>{std::move(fn)};
    }
} map{};

// Alias
inline constexpr auto& transform = map;

// ============================================================================
// FlatMap
// ============================================================================

template<typename Fn>
struct flat_map_view : pipeable_base<flat_map_view<Fn>> {
    Fn fn;
    
    constexpr explicit flat_map_view(Fn f) : fn(std::move(f)) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        return std::forward<R>(r) 
            | ::ranges::views::transform(fn) 
            | ::ranges::views::join;
    }
};

inline constexpr struct flat_map_fn {
    template<typename Fn>
    [[nodiscard]] constexpr auto operator()(Fn fn) const {
        return flat_map_view<Fn>{std::move(fn)};
    }
} flat_map{};

// ============================================================================
// Flatten
// ============================================================================

inline constexpr struct flatten_fn {
    [[nodiscard]] constexpr auto operator()() const {
        return make_closure([](auto&& r) {
            return std::forward<decltype(r)>(r) | ::ranges::views::join;
        });
    }
} flatten{};

// ============================================================================
// Reverse
// ============================================================================

inline constexpr struct reverse_range_fn {
    [[nodiscard]] constexpr auto operator()() const {
        return make_closure([](auto&& r) {
            return std::forward<decltype(r)>(r) | ::ranges::views::reverse;
        });
    }
} reverse_range{};

// ============================================================================
// Tap (side effect without modifying)
// ============================================================================

template<typename Fn>
struct tap_view : pipeable_base<tap_view<Fn>> {
    Fn fn;
    
    constexpr explicit tap_view(Fn f) : fn(std::move(f)) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        return std::forward<R>(r) | ::ranges::views::transform(
            [f=fn](auto&& x) -> decltype(auto) {
                f(x);
                return std::forward<decltype(x)>(x);
            });
    }
};

inline constexpr struct tap_fn {
    template<typename Fn>
    [[nodiscard]] constexpr auto operator()(Fn fn) const {
        return tap_view<Fn>{std::move(fn)};
    }
} tap{};

// ============================================================================
// Filter + Map combined (filter_map) - lazy version
// ============================================================================

template<typename Fn>
struct filter_map_view : pipeable_base<filter_map_view<Fn>> {
    Fn fn;  // Returns optional<T>
    
    constexpr explicit filter_map_view(Fn f) : fn(std::move(f)) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        // Transform to optional, filter those with value, then unwrap
        return std::forward<R>(r) 
            | ::ranges::views::transform(fn)
            | ::ranges::views::filter([](const auto& opt) { return opt.has_value(); })
            | ::ranges::views::transform([](auto&& opt) -> decltype(auto) { return *std::forward<decltype(opt)>(opt); });
    }
};

inline constexpr struct filter_map_fn {
    template<typename Fn>
    [[nodiscard]] constexpr auto operator()(Fn fn) const {
        return filter_map_view<Fn>{std::move(fn)};
    }
} filter_map{};

// ============================================================================
// Intersperse
// ============================================================================

template<typename T>
struct intersperse_view : pipeable_base<intersperse_view<T>> {
    T separator;
    
    constexpr explicit intersperse_view(T sep) : separator(std::move(sep)) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        return std::forward<R>(r) | ::ranges::views::intersperse(separator);
    }
};

inline constexpr struct intersperse_fn {
    template<typename T>
    [[nodiscard]] constexpr auto operator()(T separator) const {
        return intersperse_view<T>{std::move(separator)};
    }
} intersperse{};

// ============================================================================
// Scan (running fold) - lazy version with shared state
// ============================================================================

template<typename BinaryOp, typename Init>
struct scan_view : pipeable_base<scan_view<BinaryOp, Init>> {
    BinaryOp op;
    Init init;
    
    constexpr scan_view(BinaryOp o, Init i) : op(std::move(o)), init(std::move(i)) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        using T = decltype(op(init, std::declval<range_value_t<R>>()));
        auto acc = std::make_shared<T>(init);
        auto operation = op;
        
        return std::forward<R>(r) | ::ranges::views::transform(
            [acc, operation](auto&& item) mutable {
                *acc = operation(std::move(*acc), std::forward<decltype(item)>(item));
                return *acc;
            });
    }
};

inline constexpr struct scan_fn {
    template<typename BinaryOp, typename Init>
    [[nodiscard]] constexpr auto operator()(Init init, BinaryOp op) const {
        return scan_view<BinaryOp, Init>{std::move(op), std::move(init)};
    }
} scan{};

// ============================================================================
// Repeat
// ============================================================================

template<typename T>
struct repeat_view : pipeable_base<repeat_view<T>> {
    std::size_t n;
    
    constexpr explicit repeat_view(std::size_t count) : n(count) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        using V = range_value_t<R>;
        auto items = std::forward<R>(r) | ::ranges::to<std::vector<V>>();
        
        std::vector<V> result;
        result.reserve(items.size() * n);
        for (std::size_t i = 0; i < n; ++i) {
            for (const auto& item : items) {
                result.push_back(item);
            }
        }
        return result;
    }
};

inline constexpr struct repeat_fn {
    [[nodiscard]] constexpr auto operator()(std::size_t n) const {
        return repeat_view<void>{n};
    }
} repeat{};

// ============================================================================
// Shuffle
// ============================================================================

template<typename RNG = std::mt19937>
struct shuffle_view : pipeable_base<shuffle_view<RNG>> {
    mutable RNG rng;
    
    shuffle_view() : rng(std::random_device{}()) {}
    explicit shuffle_view(RNG r) : rng(std::move(r)) {}
    explicit shuffle_view(typename RNG::result_type seed) : rng(seed) {}
    
    template<input_range R>
    auto operator()(R&& r) const {
        auto result = std::forward<R>(r) | ::ranges::to<std::vector>();
        std::shuffle(result.begin(), result.end(), rng);
        return result;
    }
};

inline constexpr struct shuffle_fn {
    [[nodiscard]] auto operator()() const {
        return shuffle_view<>{};
    }
    
    [[nodiscard]] auto operator()(unsigned int seed) const {
        return shuffle_view<std::mt19937>{seed};
    }
    
    template<typename RNG>
    [[nodiscard]] auto operator()(RNG rng) const {
        return shuffle_view<RNG>{std::move(rng)};
    }
} shuffle{};

// ============================================================================
// Sample (random sampling)
// ============================================================================

template<typename RNG = std::mt19937>
struct sample_view : pipeable_base<sample_view<RNG>> {
    std::size_t n;
    mutable RNG rng;
    
    sample_view(std::size_t count) : n(count), rng(std::random_device{}()) {}
    sample_view(std::size_t count, RNG r) : n(count), rng(std::move(r)) {}
    sample_view(std::size_t count, typename RNG::result_type seed) : n(count), rng(seed) {}
    
    template<input_range R>
    auto operator()(R&& r) const {
        auto items = std::forward<R>(r) | ::ranges::to<std::vector>();
        
        if (n >= items.size()) return items;
        
        std::vector<range_value_t<R>> result;
        result.reserve(n);
        std::sample(items.begin(), items.end(), std::back_inserter(result), n, rng);
        return result;
    }
};

inline constexpr struct sample_fn {
    [[nodiscard]] auto operator()(std::size_t n) const {
        return sample_view<>{n};
    }
    
    [[nodiscard]] auto operator()(std::size_t n, unsigned int seed) const {
        return sample_view<std::mt19937>{n, seed};
    }
} sample{};

} // namespace rpp
