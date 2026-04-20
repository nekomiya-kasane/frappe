#pragma once

#include <concepts>
#include <functional>
#include <range/v3/range/concepts.hpp>
#include <ranges>
#include <type_traits>

namespace rpp {

// ============================================================================
// Core concepts
// ============================================================================

template <typename Proj, typename T>
concept projection_for = std::invocable<Proj, T> && !std::same_as<std::invoke_result_t<Proj, T>, void>;

template <typename Pred, typename T>
concept predicate_for = std::predicate<Pred, T>;

template <typename Comp, typename T>
concept comparator_for = std::relation<Comp, T, T>;

template <typename F, typename T, typename U>
concept binary_invocable = std::invocable<F, T, U>;

// ============================================================================
// Range concepts (compatible with std::ranges and range-v3)
// ============================================================================

template <typename R>
concept input_range = std::ranges::input_range<R> || ::ranges::input_range<R>;

template <typename R>
concept forward_range = std::ranges::forward_range<R> || ::ranges::forward_range<R>;

template <typename R>
concept bidirectional_range = std::ranges::bidirectional_range<R> || ::ranges::bidirectional_range<R>;

template <typename R>
concept random_access_range = std::ranges::random_access_range<R> || ::ranges::random_access_range<R>;

template <typename R>
concept sized_range = std::ranges::sized_range<R> || ::ranges::sized_range<R>;

// ============================================================================
// Range value type extraction
// ============================================================================

template <typename R> using range_value_t = std::ranges::range_value_t<R>;

template <typename R> using range_reference_t = std::ranges::range_reference_t<R>;

// ============================================================================
// Adapter concepts
// ============================================================================

template <typename A, typename R>
concept lazy_adapter_for = input_range<R> && requires(A a, R &&r) {
    { a(std::forward<R>(r)) } -> input_range;
};

template <typename A, typename R>
concept eager_adapter_for = input_range<R> && requires(A a, R &&r) {
    { a(std::forward<R>(r)) };
};

template <typename A, typename R>
concept terminal_adapter_for = input_range<R> && requires(A a, R &&r) { a(std::forward<R>(r)); };

// ============================================================================
// Hashable concept for join/set operations
// ============================================================================

template <typename T>
concept hashable = requires(T t) {
    { std::hash<T>{}(t) } -> std::convertible_to<std::size_t>;
};

// ============================================================================
// Optional-like concept
// ============================================================================

template <typename T>
concept optional_like = requires(T t) {
    { t.has_value() } -> std::convertible_to<bool>;
    { *t };
};

} // namespace rpp
