#pragma once

#include "concepts.hpp"
#include "pipeable.hpp"

#include <functional>
#include <range/v3/view/filter.hpp>

namespace rpp {

    // ============================================================================
    // Generic filter adapter
    // ============================================================================

    template <typename Pred> struct filter_view : pipeable_base<filter_view<Pred>> {
        Pred pred;

        constexpr explicit filter_view(Pred p) : pred(std::move(p)) {}

        template <input_range R>
            requires predicate_for<Pred, range_value_t<R>>
        constexpr auto operator()(R &&r) const {
            return std::forward<R>(r) | ::ranges::views::filter(pred);
        }
    };

    template <typename Pred> filter_view(Pred) -> filter_view<Pred>;

    // CPO: rpp::filter - create a filter adapter
    inline constexpr struct filter_fn {
        template <typename Pred> [[nodiscard]] constexpr auto operator()(Pred pred) const {
            return filter_view<Pred>{std::move(pred)};
        }

        template <typename Proj, typename Pred> [[nodiscard]] constexpr auto operator()(Proj proj, Pred pred) const {
            return filter_view{
                [p = std::move(proj), pr = std::move(pred)](const auto &x) { return pr(std::invoke(p, x)); }};
        }
    } filter{};

    // ============================================================================
    // Convenience filter factories
    // ============================================================================

    namespace filters {

        template <typename Proj, typename V> [[nodiscard]] constexpr auto eq(Proj proj, V value) {
            return filter(
                [p = std::move(proj), v = std::move(value)](const auto &x) { return std::invoke(p, x) == v; });
        }

        template <typename Proj, typename V> [[nodiscard]] constexpr auto ne(Proj proj, V value) {
            return filter(
                [p = std::move(proj), v = std::move(value)](const auto &x) { return std::invoke(p, x) != v; });
        }

        template <typename Proj, typename V> [[nodiscard]] constexpr auto gt(Proj proj, V value) {
            return filter([p = std::move(proj), v = std::move(value)](const auto &x) { return std::invoke(p, x) > v; });
        }

        template <typename Proj, typename V> [[nodiscard]] constexpr auto ge(Proj proj, V value) {
            return filter(
                [p = std::move(proj), v = std::move(value)](const auto &x) { return std::invoke(p, x) >= v; });
        }

        template <typename Proj, typename V> [[nodiscard]] constexpr auto lt(Proj proj, V value) {
            return filter([p = std::move(proj), v = std::move(value)](const auto &x) { return std::invoke(p, x) < v; });
        }

        template <typename Proj, typename V> [[nodiscard]] constexpr auto le(Proj proj, V value) {
            return filter(
                [p = std::move(proj), v = std::move(value)](const auto &x) { return std::invoke(p, x) <= v; });
        }

        template <typename Proj, typename V> [[nodiscard]] constexpr auto between(Proj proj, V lo, V hi) {
            return filter([p = std::move(proj), lo = std::move(lo), hi = std::move(hi)](const auto &x) {
                auto v = std::invoke(p, x);
                return v >= lo && v <= hi;
            });
        }

        template <typename Proj, typename Container> [[nodiscard]] constexpr auto in(Proj proj, Container values) {
            return filter([p = std::move(proj), vs = std::move(values)](const auto &x) {
                auto v = std::invoke(p, x);
                return std::find(vs.begin(), vs.end(), v) != vs.end();
            });
        }

        template <typename Proj, typename Container> [[nodiscard]] constexpr auto not_in(Proj proj, Container values) {
            return filter([p = std::move(proj), vs = std::move(values)](const auto &x) {
                auto v = std::invoke(p, x);
                return std::find(vs.begin(), vs.end(), v) == vs.end();
            });
        }

        template <typename Proj, typename V> [[nodiscard]] constexpr auto contains(Proj proj, V substr) {
            return filter([p = std::move(proj), s = std::move(substr)](const auto &x) {
                auto str = std::invoke(p, x);
                return str.find(s) != std::string::npos;
            });
        }

        template <typename Proj> [[nodiscard]] constexpr auto starts_with(Proj proj, std::string_view prefix) {
            return filter([p = std::move(proj), pf = std::string(prefix)](const auto &x) {
                auto s = std::invoke(p, x);
                return s.starts_with(pf);
            });
        }

        template <typename Proj> [[nodiscard]] constexpr auto ends_with(Proj proj, std::string_view suffix) {
            return filter([p = std::move(proj), sf = std::string(suffix)](const auto &x) {
                auto s = std::invoke(p, x);
                return s.ends_with(sf);
            });
        }

        template <typename Proj> [[nodiscard]] constexpr auto is_null(Proj proj) {
            return filter([p = std::move(proj)](const auto &x) { return !std::invoke(p, x).has_value(); });
        }

        template <typename Proj> [[nodiscard]] constexpr auto is_not_null(Proj proj) {
            return filter([p = std::move(proj)](const auto &x) { return std::invoke(p, x).has_value(); });
        }

    } // namespace filters

} // namespace rpp
