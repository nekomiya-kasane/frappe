#pragma once

#include "filter.hpp"

#include <regex>
#include <string>
#include <string_view>
#include <variant>

namespace rpp::filters {

// ============================================================================
// SQL LIKE pattern matching
// ============================================================================

namespace detail {
inline std::string like_to_regex(std::string_view pattern) {
    std::string regex_pattern;
    regex_pattern.reserve(pattern.size() * 2);

    for (char c : pattern) {
        switch (c) {
        case '%':
            regex_pattern += ".*";
            break;
        case '_':
            regex_pattern += ".";
            break;
        case '.':
        case '\\':
        case '+':
        case '*':
        case '?':
        case '[':
        case ']':
        case '(':
        case ')':
        case '{':
        case '}':
        case '^':
        case '$':
        case '|':
            regex_pattern += '\\';
            regex_pattern += c;
            break;
        default:
            regex_pattern += c;
        }
    }
    return regex_pattern;
}
} // namespace detail

template <typename Proj> [[nodiscard]] inline auto like(Proj proj, std::string_view pattern) {
    auto regex_pattern = detail::like_to_regex(pattern);
    return filter([p = std::move(proj), re = std::regex(regex_pattern)](const auto &x) {
        auto s = std::invoke(p, x);
        return std::regex_match(s.begin(), s.end(), re);
    });
}

template <typename Proj> [[nodiscard]] inline auto not_like(Proj proj, std::string_view pattern) {
    auto regex_pattern = detail::like_to_regex(pattern);
    return filter([p = std::move(proj), re = std::regex(regex_pattern)](const auto &x) {
        auto s = std::invoke(p, x);
        return !std::regex_match(s.begin(), s.end(), re);
    });
}

// ============================================================================
// Regex matching
// ============================================================================

template <typename Proj> [[nodiscard]] inline auto matches(Proj proj, std::string_view pattern) {
    return filter([p = std::move(proj), re = std::regex(std::string(pattern))](const auto &x) {
        auto s = std::invoke(p, x);
        return std::regex_search(s.begin(), s.end(), re);
    });
}

template <typename Proj> [[nodiscard]] inline auto matches(Proj proj, const std::regex &re) {
    return filter([p = std::move(proj), re](const auto &x) {
        auto s = std::invoke(p, x);
        return std::regex_search(s.begin(), s.end(), re);
    });
}

template <typename Proj> [[nodiscard]] inline auto full_match(Proj proj, std::string_view pattern) {
    return filter([p = std::move(proj), re = std::regex(std::string(pattern))](const auto &x) {
        auto s = std::invoke(p, x);
        return std::regex_match(s.begin(), s.end(), re);
    });
}

// ============================================================================
// Type-based filters
// ============================================================================

template <typename T, typename Proj> [[nodiscard]] constexpr auto is_type(Proj proj) {
    return filter([p = std::move(proj)](const auto &x) {
        using ResultType = std::invoke_result_t<Proj, decltype(x)>;
        if constexpr (std::is_same_v<ResultType, T>) {
            return true;
        } else {
            return std::holds_alternative<T>(std::invoke(p, x));
        }
    });
}

// ============================================================================
// Predicate combinators
// ============================================================================

template <typename... Preds> [[nodiscard]] constexpr auto all_of(Preds... preds) {
    return filter([... ps = std::move(preds)](const auto &x) { return (ps(x) && ...); });
}

template <typename... Preds> [[nodiscard]] constexpr auto any_of(Preds... preds) {
    return filter([... ps = std::move(preds)](const auto &x) { return (ps(x) || ...); });
}

template <typename... Preds> [[nodiscard]] constexpr auto none_of(Preds... preds) {
    return filter([... ps = std::move(preds)](const auto &x) { return !(ps(x) || ...); });
}

} // namespace rpp::filters
