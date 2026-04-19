#pragma once

#include "filter.hpp"

namespace rpp {

// ============================================================================
// Predicate combinators
// ============================================================================

template<typename P1, typename P2>
struct and_pred {
    P1 p1;
    P2 p2;
    
    template<typename T>
    constexpr bool operator()(const T& x) const {
        return p1(x) && p2(x);
    }
};

template<typename P1, typename P2>
struct or_pred {
    P1 p1;
    P2 p2;
    
    template<typename T>
    constexpr bool operator()(const T& x) const {
        return p1(x) || p2(x);
    }
};

template<typename P>
struct not_pred {
    P p;
    
    template<typename T>
    constexpr bool operator()(const T& x) const {
        return !p(x);
    }
};

// ============================================================================
// Filter combination operators
// ============================================================================

template<typename P1, typename P2>
[[nodiscard]] constexpr auto operator&&(filter_view<P1> f1, filter_view<P2> f2) {
    return filter_view{and_pred<P1, P2>{std::move(f1.pred), std::move(f2.pred)}};
}

template<typename P1, typename P2>
[[nodiscard]] constexpr auto operator||(filter_view<P1> f1, filter_view<P2> f2) {
    return filter_view{or_pred<P1, P2>{std::move(f1.pred), std::move(f2.pred)}};
}

template<typename P>
[[nodiscard]] constexpr auto operator!(filter_view<P> f) {
    return filter_view{not_pred<P>{std::move(f.pred)}};
}

// ============================================================================
// Functional combinators
// ============================================================================

template<typename F1, typename F2>
[[nodiscard]] constexpr auto and_(F1 f1, F2 f2) {
    return filter_view{and_pred<typename F1::pred_type, typename F2::pred_type>{
        std::move(f1.pred), std::move(f2.pred)
    }};
}

template<typename F1, typename F2>
[[nodiscard]] constexpr auto or_(F1 f1, F2 f2) {
    return filter_view{or_pred<typename F1::pred_type, typename F2::pred_type>{
        std::move(f1.pred), std::move(f2.pred)
    }};
}

template<typename F>
[[nodiscard]] constexpr auto not_(F f) {
    return filter_view{not_pred<typename F::pred_type>{std::move(f.pred)}};
}

} // namespace rpp
