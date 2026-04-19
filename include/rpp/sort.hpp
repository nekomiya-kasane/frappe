#pragma once

#include "concepts.hpp"
#include "pipeable.hpp"
#include "comparator.hpp"
#include <range/v3/algorithm/sort.hpp>
#include <range/v3/algorithm/stable_sort.hpp>
#include <range/v3/range/conversion.hpp>
#include <vector>
#include <functional>

namespace rpp {

// ============================================================================
// Generic sort adapter (eager evaluation, returns owning_view for continued piping)
// ============================================================================

template<typename Proj = std::identity, typename Comp = std::less<>>
struct sort_view : pipeable_base<sort_view<Proj, Comp>> {
    Proj proj;
    Comp comp;
    bool descending = false;
    
    constexpr sort_view(Proj p = {}, Comp c = {}, bool desc = false)
        : proj(std::move(p)), comp(std::move(c)), descending(desc) {}
    
    template<input_range R>
        requires projection_for<Proj, range_value_t<R>>
    constexpr auto operator()(R&& r) const {
        using T = range_value_t<R>;
        auto result = std::forward<R>(r) | ::ranges::to<std::vector<T>>();
        
        if (descending) {
            ::ranges::sort(result, [this](const T& a, const T& b) {
                return comp(std::invoke(proj, b), std::invoke(proj, a));
            });
        } else {
            ::ranges::sort(result, [this](const T& a, const T& b) {
                return comp(std::invoke(proj, a), std::invoke(proj, b));
            });
        }
        return owning_view<std::vector<T>>{std::move(result)};
    }
    
    [[nodiscard]] constexpr auto desc() const {
        return sort_view<Proj, Comp>{proj, comp, true};
    }
    
    [[nodiscard]] constexpr auto asc() const {
        return sort_view<Proj, Comp>{proj, comp, false};
    }
    
    [[nodiscard]] constexpr auto as_comparator() const {
        return comparator<Proj, Comp>{proj, comp, descending};
    }
};

template<typename Proj, typename Comp>
sort_view(Proj, Comp, bool) -> sort_view<Proj, Comp>;

template<typename Proj>
sort_view(Proj) -> sort_view<Proj, std::less<>>;

// CPO: rpp::sort_by
inline constexpr struct sort_by_fn {
    template<typename Proj, typename Comp = std::less<>>
    [[nodiscard]] constexpr auto operator()(Proj proj, Comp comp = {}) const {
        return sort_view<Proj, Comp>{std::move(proj), std::move(comp), false};
    }
} sort_by{};

// CPO: rpp::sort (default sort by value)
inline constexpr struct sort_fn {
    [[nodiscard]] constexpr auto operator()() const {
        return sort_view<>{};
    }
    
    template<typename Comp>
    [[nodiscard]] constexpr auto operator()(Comp comp) const {
        return sort_view<std::identity, Comp>{{}, std::move(comp), false};
    }
} sort{};

// ============================================================================
// Stable sort
// ============================================================================

template<typename Proj = std::identity, typename Comp = std::less<>>
struct stable_sort_view : pipeable_base<stable_sort_view<Proj, Comp>> {
    Proj proj;
    Comp comp;
    bool descending = false;
    
    constexpr stable_sort_view(Proj p = {}, Comp c = {}, bool desc = false)
        : proj(std::move(p)), comp(std::move(c)), descending(desc) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        using T = range_value_t<R>;
        auto result = std::forward<R>(r) | ::ranges::to<std::vector<T>>();
        
        if (descending) {
            ::ranges::stable_sort(result, [this](const T& a, const T& b) {
                return comp(std::invoke(proj, b), std::invoke(proj, a));
            });
        } else {
            ::ranges::stable_sort(result, [this](const T& a, const T& b) {
                return comp(std::invoke(proj, a), std::invoke(proj, b));
            });
        }
        return owning_view<std::vector<T>>{std::move(result)};
    }
    
    [[nodiscard]] constexpr auto desc() const {
        return stable_sort_view<Proj, Comp>{proj, comp, true};
    }
    
    [[nodiscard]] constexpr auto asc() const {
        return stable_sort_view<Proj, Comp>{proj, comp, false};
    }
};

inline constexpr struct stable_sort_by_fn {
    template<typename Proj, typename Comp = std::less<>>
    [[nodiscard]] constexpr auto operator()(Proj proj, Comp comp = {}) const {
        return stable_sort_view<Proj, Comp>{std::move(proj), std::move(comp), false};
    }
} stable_sort_by{};

inline constexpr struct stable_sort_fn {
    [[nodiscard]] constexpr auto operator()() const {
        return stable_sort_view<>{};
    }
} stable_sort{};

// ============================================================================
// Natural sort (numeric-aware)
// ============================================================================

template<typename Proj = std::identity>
struct natural_sort_view : pipeable_base<natural_sort_view<Proj>> {
    Proj proj;
    bool descending = false;
    
    constexpr natural_sort_view(Proj p = {}, bool desc = false)
        : proj(std::move(p)), descending(desc) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        using T = range_value_t<R>;
        auto result = std::forward<R>(r) | ::ranges::to<std::vector<T>>();
        
        if (descending) {
            ::ranges::sort(result, [this](const T& a, const T& b) {
                return natural_comparator::natural_compare(
                    std::invoke(proj, b), std::invoke(proj, a));
            });
        } else {
            ::ranges::sort(result, [this](const T& a, const T& b) {
                return natural_comparator::natural_compare(
                    std::invoke(proj, a), std::invoke(proj, b));
            });
        }
        return owning_view<std::vector<T>>{std::move(result)};
    }
    
    [[nodiscard]] constexpr auto desc() const {
        return natural_sort_view<Proj>{proj, true};
    }
    
    [[nodiscard]] constexpr auto asc() const {
        return natural_sort_view<Proj>{proj, false};
    }
};

inline constexpr struct natural_sort_fn {
    [[nodiscard]] constexpr auto operator()() const {
        return natural_sort_view<>{};
    }
    
    template<typename Proj>
    [[nodiscard]] constexpr auto operator()(Proj proj) const {
        return natural_sort_view<Proj>{std::move(proj), false};
    }
} natural_sort{};

// ============================================================================
// Multi-level sort
// ============================================================================

template<typename... Projs>
struct multi_sort_view : pipeable_base<multi_sort_view<Projs...>> {
    std::tuple<Projs...> projs;
    
    constexpr explicit multi_sort_view(Projs... ps) 
        : projs(std::move(ps)...) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        using T = range_value_t<R>;
        auto result = std::forward<R>(r) | ::ranges::to<std::vector<T>>();
        
        ::ranges::sort(result, [this](const T& a, const T& b) {
            return compare_impl(a, b, std::index_sequence_for<Projs...>{});
        });
        
        return owning_view<std::vector<T>>{std::move(result)};
    }
    
private:
    template<typename T, std::size_t... Is>
    constexpr bool compare_impl(const T& a, const T& b, std::index_sequence<Is...>) const {
        bool result = false;
        bool decided = false;
        ((decided || (
            invoke_compare(std::get<Is>(projs), a, b, result, decided)
        )), ...);
        return result;
    }
    
    template<typename P, typename T>
    static constexpr bool invoke_compare(const P& p, const T& a, const T& b, 
                                         bool& result, bool& decided) {
        if constexpr (requires { p.descending; }) {
            auto va = std::invoke(p.proj, a);
            auto vb = std::invoke(p.proj, b);
            if (p.descending) {
                if (vb < va) { result = true; decided = true; }
                else if (va < vb) { decided = true; }
            } else {
                if (va < vb) { result = true; decided = true; }
                else if (vb < va) { decided = true; }
            }
        } else {
            auto va = std::invoke(p, a);
            auto vb = std::invoke(p, b);
            if (va < vb) { result = true; decided = true; }
            else if (vb < va) { decided = true; }
        }
        return false;
    }
};

template<typename... Projs>
[[nodiscard]] constexpr auto multi_sort(Projs... projs) {
    return multi_sort_view<Projs...>{std::move(projs)...};
}

// ============================================================================
// Partial sort (top/bottom N elements)
// ============================================================================

template<typename Proj = std::identity, typename Comp = std::less<>>
struct partial_sort_view : pipeable_base<partial_sort_view<Proj, Comp>> {
    Proj proj;
    Comp comp;
    std::size_t n;
    bool descending = false;
    
    constexpr partial_sort_view(std::size_t count, Proj p = {}, Comp c = {}, bool desc = false)
        : proj(std::move(p)), comp(std::move(c)), n(count), descending(desc) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        using T = range_value_t<R>;
        auto result = std::forward<R>(r) | ::ranges::to<std::vector<T>>();
        
        auto count = std::min(n, result.size());
        
        if (descending) {
            std::partial_sort(result.begin(), result.begin() + count, result.end(),
                [this](const T& a, const T& b) {
                    return comp(std::invoke(proj, b), std::invoke(proj, a));
                });
        } else {
            std::partial_sort(result.begin(), result.begin() + count, result.end(),
                [this](const T& a, const T& b) {
                    return comp(std::invoke(proj, a), std::invoke(proj, b));
                });
        }
        
        result.resize(count);
        return owning_view<std::vector<T>>{std::move(result)};
    }
};

inline constexpr struct top_n_fn {
    template<typename Proj = std::identity, typename Comp = std::less<>>
    [[nodiscard]] constexpr auto operator()(std::size_t n, Proj proj = {}, Comp comp = {}) const {
        return partial_sort_view<Proj, Comp>{n, std::move(proj), std::move(comp), true};
    }
} top_n{};

inline constexpr struct bottom_n_fn {
    template<typename Proj = std::identity, typename Comp = std::less<>>
    [[nodiscard]] constexpr auto operator()(std::size_t n, Proj proj = {}, Comp comp = {}) const {
        return partial_sort_view<Proj, Comp>{n, std::move(proj), std::move(comp), false};
    }
} bottom_n{};

} // namespace rpp
