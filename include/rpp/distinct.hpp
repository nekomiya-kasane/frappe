#pragma once

#include "concepts.hpp"
#include "pipeable.hpp"
#include <range/v3/view/unique.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/range/conversion.hpp>
#include <unordered_set>
#include <memory>
#include <vector>

namespace rpp {

// ============================================================================
// Distinct (order-preserving, lazy evaluation)
// ============================================================================

template<typename Proj = std::identity>
struct distinct_view : pipeable_base<distinct_view<Proj>> {
    Proj proj;
    
    constexpr explicit distinct_view(Proj p = {}) : proj(std::move(p)) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        using T = range_value_t<R>;
        using K = std::decay_t<std::invoke_result_t<Proj, T>>;
        
        std::unordered_set<K> seen;
        std::vector<T> result;
        for (auto&& item : r) {
            auto key = std::invoke(proj, item);
            if (seen.insert(key).second) {
                result.push_back(std::forward<decltype(item)>(item));
            }
        }
        return owning_view<std::vector<T>>{std::move(result)};
    }
};

template<typename Proj>
distinct_view(Proj) -> distinct_view<Proj>;

inline constexpr struct distinct_fn {
    [[nodiscard]] constexpr auto operator()() const {
        return distinct_view<>{};
    }
    
    template<typename Proj>
    [[nodiscard]] constexpr auto operator()(Proj proj) const {
        return distinct_view<Proj>{std::move(proj)};
    }
} distinct{};

inline constexpr auto& unique = distinct;

// ============================================================================
// Unique consecutive (sorted range, lazy)
// ============================================================================

inline constexpr struct unique_consecutive_fn {
    [[nodiscard]] constexpr auto operator()() const {
        return make_closure([](auto&& r) {
            return std::forward<decltype(r)>(r) | ::ranges::views::unique;
        });
    }
    
    template<typename Proj>
    [[nodiscard]] constexpr auto operator()(Proj proj) const {
        return make_closure([p=std::move(proj)](auto&& r) {
            return std::forward<decltype(r)>(r) | ::ranges::views::unique(
                [p](const auto& a, const auto& b) {
                    return std::invoke(p, a) == std::invoke(p, b);
                });
        });
    }
} unique_consecutive{};

// ============================================================================
// Distinct with count
// ============================================================================

template<typename Proj = std::identity>
struct distinct_count_view : pipeable_base<distinct_count_view<Proj>> {
    Proj proj;
    
    constexpr explicit distinct_count_view(Proj p = {}) : proj(std::move(p)) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        using T = range_value_t<R>;
        using K = std::decay_t<std::invoke_result_t<Proj, T>>;
        
        std::unordered_map<K, std::pair<T, std::size_t>> seen;
        std::vector<K> order;
        
        for (auto&& item : r) {
            auto key = std::invoke(proj, item);
            auto it = seen.find(key);
            if (it == seen.end()) {
                seen.emplace(key, std::make_pair(std::forward<decltype(item)>(item), 1));
                order.push_back(key);
            } else {
                ++it->second.second;
            }
        }
        
        std::vector<std::pair<T, std::size_t>> result;
        result.reserve(order.size());
        for (const auto& key : order) {
            result.push_back(std::move(seen[key]));
        }
        return result;
    }
};

inline constexpr struct distinct_count_fn {
    [[nodiscard]] constexpr auto operator()() const {
        return distinct_count_view<>{};
    }
    
    template<typename Proj>
    [[nodiscard]] constexpr auto operator()(Proj proj) const {
        return distinct_count_view<Proj>{std::move(proj)};
    }
} distinct_count{};

} // namespace rpp
