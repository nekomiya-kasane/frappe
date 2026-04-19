#pragma once

#include "concepts.hpp"
#include "pipeable.hpp"
#include <range/v3/view/transform.hpp>
#include <tuple>

namespace rpp {

// ============================================================================
// Multi-column select: select(proj1, proj2, ...) -> range<tuple<T1, T2, ...>>
// ============================================================================

template<typename... Projs>
struct select_view : pipeable_base<select_view<Projs...>> {
    std::tuple<Projs...> projs;
    
    constexpr explicit select_view(Projs... ps) : projs(std::move(ps)...) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        return std::forward<R>(r) | ::ranges::views::transform([this](const auto& x) {
            return std::apply([&x](const auto&... ps) {
                return std::make_tuple(std::invoke(ps, x)...);
            }, projs);
        });
    }
};

template<typename... Projs>
select_view(Projs...) -> select_view<Projs...>;

inline constexpr struct select_fn {
    template<typename... Projs>
    [[nodiscard]] constexpr auto operator()(Projs... projs) const {
        return select_view<Projs...>{std::move(projs)...};
    }
} select{};

// ============================================================================
// Single-column select (returns value, not tuple)
// ============================================================================

template<typename Proj>
struct pluck_view : pipeable_base<pluck_view<Proj>> {
    Proj proj;
    
    constexpr explicit pluck_view(Proj p) : proj(std::move(p)) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        return std::forward<R>(r) | ::ranges::views::transform(
            [p=proj](const auto& x) { return std::invoke(p, x); });
    }
};

template<typename Proj>
pluck_view(Proj) -> pluck_view<Proj>;

inline constexpr struct pluck_fn {
    template<typename Proj>
    [[nodiscard]] constexpr auto operator()(Proj proj) const {
        return pluck_view<Proj>{std::move(proj)};
    }
} pluck{};

// ============================================================================
// Add computed column
// ============================================================================

template<typename Proj>
struct with_view : pipeable_base<with_view<Proj>> {
    Proj proj;
    
    constexpr explicit with_view(Proj p) : proj(std::move(p)) {}
    
    template<input_range R>
    constexpr auto operator()(R&& r) const {
        return std::forward<R>(r) | ::ranges::views::transform(
            [p=proj](auto&& x) {
                return std::make_pair(std::forward<decltype(x)>(x), std::invoke(p, x));
            });
    }
};

inline constexpr struct with_fn {
    template<typename Proj>
    [[nodiscard]] constexpr auto operator()(Proj proj) const {
        return with_view<Proj>{std::move(proj)};
    }
} with{};

// ============================================================================
// CASE WHEN conditional expression
// ============================================================================

template<typename Cond, typename Then, typename Else>
struct case_when_proj {
    Cond cond;
    Then then_proj;
    Else else_proj;
    
    template<typename T>
    constexpr auto operator()(const T& x) const {
        if (cond(x)) {
            return std::invoke(then_proj, x);
        } else {
            return std::invoke(else_proj, x);
        }
    }
};

inline constexpr struct case_when_fn {
    template<typename Cond, typename Then, typename Else>
    [[nodiscard]] constexpr auto operator()(Cond cond, Then then_proj, Else else_proj) const {
        return case_when_proj<Cond, Then, Else>{
            std::move(cond), std::move(then_proj), std::move(else_proj)
        };
    }
} case_when{};

// ============================================================================
// COALESCE - null value replacement
// ============================================================================

template<typename Proj, typename Default>
struct coalesce_proj {
    Proj proj;
    Default default_value;
    
    template<typename T>
    constexpr auto operator()(const T& x) const {
        auto result = std::invoke(proj, x);
        if constexpr (requires { result.has_value(); }) {
            return result.has_value() ? *result : default_value;
        } else {
            return result ? result : default_value;
        }
    }
};

inline constexpr struct coalesce_fn {
    template<typename Proj, typename Default>
    [[nodiscard]] constexpr auto operator()(Proj proj, Default default_value) const {
        return coalesce_proj<Proj, Default>{std::move(proj), std::move(default_value)};
    }
} coalesce{};

} // namespace rpp
