#pragma once

#include "concepts.hpp"
#include "pipeable.hpp"
#include <range/v3/view/concat.hpp>
#include <range/v3/range/conversion.hpp>
#include <unordered_set>
#include <vector>

namespace rpp {

// ============================================================================
// Concat (UNION ALL) - lazy version using range-v3 concat
// ============================================================================

inline constexpr struct concat_fn {
    template<input_range R2>
    [[nodiscard]] constexpr auto operator()(R2&& other) const {
        return make_closure([o = std::forward<R2>(other) | ::ranges::to<std::vector>()](auto&& r1) {
            return ::ranges::views::concat(std::forward<decltype(r1)>(r1), o);
        });
    }
} concat{};

// ============================================================================
// Union with deduplication (UNION)
// ============================================================================

template<typename R2, typename Proj = std::identity>
struct union_view : pipeable_base<union_view<R2, Proj>> {
    R2 other;
    Proj proj;
    
    constexpr union_view(R2 o, Proj p = {}) : other(std::move(o)), proj(std::move(p)) {}
    
    template<input_range R1>
    constexpr auto operator()(R1&& r1) const {
        using T = range_value_t<R1>;
        using K = std::decay_t<std::invoke_result_t<Proj, T>>;
        
        std::unordered_set<K> seen;
        std::vector<T> result;
        
        for (auto&& item : r1) {
            auto key = std::invoke(proj, item);
            if (seen.insert(key).second) {
                result.push_back(std::forward<decltype(item)>(item));
            }
        }
        for (auto&& item : other) {
            auto key = std::invoke(proj, item);
            if (seen.insert(key).second) {
                result.push_back(std::forward<decltype(item)>(item));
            }
        }
        return result;
    }
};

inline constexpr struct union_fn {
    template<input_range R2>
    [[nodiscard]] constexpr auto operator()(R2&& other) const {
        using R2Decayed = std::decay_t<R2>;
        return union_view<R2Decayed>{std::forward<R2>(other) | ::ranges::to<std::vector>()};
    }
    
    template<input_range R2, typename Proj>
    [[nodiscard]] constexpr auto operator()(R2&& other, Proj proj) const {
        using R2Decayed = std::decay_t<R2>;
        return union_view<R2Decayed, Proj>{
            std::forward<R2>(other) | ::ranges::to<std::vector>(),
            std::move(proj)
        };
    }
} union_{};

// ============================================================================
// Intersection (INTERSECT)
// ============================================================================

template<typename R2, typename Proj = std::identity>
struct intersect_view : pipeable_base<intersect_view<R2, Proj>> {
    R2 other;
    Proj proj;
    
    constexpr intersect_view(R2 o, Proj p = {}) : other(std::move(o)), proj(std::move(p)) {}
    
    template<input_range R1>
    constexpr auto operator()(R1&& r1) const {
        using T = range_value_t<R1>;
        using K = std::decay_t<std::invoke_result_t<Proj, T>>;
        
        std::unordered_set<K> other_keys;
        for (const auto& item : other) {
            other_keys.insert(std::invoke(proj, item));
        }
        
        std::unordered_set<K> seen;
        std::vector<T> result;
        for (auto&& item : r1) {
            auto key = std::invoke(proj, item);
            if (other_keys.contains(key) && seen.insert(key).second) {
                result.push_back(std::forward<decltype(item)>(item));
            }
        }
        return result;
    }
};

inline constexpr struct intersect_fn {
    template<input_range R2>
    [[nodiscard]] constexpr auto operator()(R2&& other) const {
        using R2Decayed = std::decay_t<R2>;
        return intersect_view<R2Decayed>{std::forward<R2>(other) | ::ranges::to<std::vector>()};
    }
    
    template<input_range R2, typename Proj>
    [[nodiscard]] constexpr auto operator()(R2&& other, Proj proj) const {
        using R2Decayed = std::decay_t<R2>;
        return intersect_view<R2Decayed, Proj>{
            std::forward<R2>(other) | ::ranges::to<std::vector>(),
            std::move(proj)
        };
    }
} intersect{};

// ============================================================================
// Difference (EXCEPT)
// ============================================================================

template<typename R2, typename Proj = std::identity>
struct except_view : pipeable_base<except_view<R2, Proj>> {
    R2 other;
    Proj proj;
    
    constexpr except_view(R2 o, Proj p = {}) : other(std::move(o)), proj(std::move(p)) {}
    
    template<input_range R1>
    constexpr auto operator()(R1&& r1) const {
        using T = range_value_t<R1>;
        using K = std::decay_t<std::invoke_result_t<Proj, T>>;
        
        std::unordered_set<K> other_keys;
        for (const auto& item : other) {
            other_keys.insert(std::invoke(proj, item));
        }
        
        std::unordered_set<K> seen;
        std::vector<T> result;
        for (auto&& item : r1) {
            auto key = std::invoke(proj, item);
            if (!other_keys.contains(key) && seen.insert(key).second) {
                result.push_back(std::forward<decltype(item)>(item));
            }
        }
        return result;
    }
};

inline constexpr struct except_fn {
    template<input_range R2>
    [[nodiscard]] constexpr auto operator()(R2&& other) const {
        using R2Decayed = std::decay_t<R2>;
        return except_view<R2Decayed>{std::forward<R2>(other) | ::ranges::to<std::vector>()};
    }
    
    template<input_range R2, typename Proj>
    [[nodiscard]] constexpr auto operator()(R2&& other, Proj proj) const {
        using R2Decayed = std::decay_t<R2>;
        return except_view<R2Decayed, Proj>{
            std::forward<R2>(other) | ::ranges::to<std::vector>(),
            std::move(proj)
        };
    }
} except{};

inline constexpr auto& difference = except;

// ============================================================================
// Symmetric difference (XOR)
// ============================================================================

template<typename R2, typename Proj = std::identity>
struct symmetric_difference_view : pipeable_base<symmetric_difference_view<R2, Proj>> {
    R2 other;
    Proj proj;
    
    constexpr symmetric_difference_view(R2 o, Proj p = {}) : other(std::move(o)), proj(std::move(p)) {}
    
    template<input_range R1>
    constexpr auto operator()(R1&& r1) const {
        using T = range_value_t<R1>;
        using K = std::decay_t<std::invoke_result_t<Proj, T>>;
        
        std::unordered_map<K, T> left_map;
        for (auto&& item : r1) {
            auto key = std::invoke(proj, item);
            left_map.try_emplace(key, std::forward<decltype(item)>(item));
        }
        
        std::unordered_set<K> right_keys;
        std::vector<T> result;
        
        // Items in right but not in left
        for (auto&& item : other) {
            auto key = std::invoke(proj, item);
            right_keys.insert(key);
            if (!left_map.contains(key)) {
                result.push_back(std::forward<decltype(item)>(item));
            }
        }
        
        // Items in left but not in right
        for (auto& [key, item] : left_map) {
            if (!right_keys.contains(key)) {
                result.push_back(std::move(item));
            }
        }
        
        return result;
    }
};

inline constexpr struct symmetric_difference_fn {
    template<input_range R2>
    [[nodiscard]] constexpr auto operator()(R2&& other) const {
        using R2Decayed = std::decay_t<R2>;
        return symmetric_difference_view<R2Decayed>{
            std::forward<R2>(other) | ::ranges::to<std::vector>()
        };
    }
    
    template<input_range R2, typename Proj>
    [[nodiscard]] constexpr auto operator()(R2&& other, Proj proj) const {
        using R2Decayed = std::decay_t<R2>;
        return symmetric_difference_view<R2Decayed, Proj>{
            std::forward<R2>(other) | ::ranges::to<std::vector>(),
            std::move(proj)
        };
    }
} symmetric_difference{};

} // namespace rpp
