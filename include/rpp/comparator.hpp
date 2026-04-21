#pragma once

#include "concepts.hpp"

#include <cctype>
#include <functional>
#include <string_view>
#include <tuple>

namespace rpp {

// ============================================================================
// Comparator adapter - usable with std::set, std::map, std::priority_queue, etc.
// ============================================================================

template <typename Proj, typename Comp = std::less<>> struct comparator {
    Proj proj;
    Comp comp;
    bool descending = false;

    constexpr comparator(Proj p, Comp c = {}, bool desc = false)
        : proj(std::move(p)), comp(std::move(c)), descending(desc) {}

    template <typename T> constexpr bool operator()(const T &a, const T &b) const {
        if (descending) {
            return comp(std::invoke(proj, b), std::invoke(proj, a));
        }
        return comp(std::invoke(proj, a), std::invoke(proj, b));
    }

    [[nodiscard]] constexpr auto desc() const { return comparator<Proj, Comp>{proj, comp, true}; }

    [[nodiscard]] constexpr auto asc() const { return comparator<Proj, Comp>{proj, comp, false}; }
};

template <typename Proj, typename Comp> comparator(Proj, Comp, bool) -> comparator<Proj, Comp>;

template <typename Proj> comparator(Proj) -> comparator<Proj, std::less<>>;

// CPO: rpp::compare_by
inline constexpr struct compare_by_fn {
    template <typename Proj, typename Comp = std::less<>>
    [[nodiscard]] constexpr auto operator()(Proj proj, Comp comp = {}) const {
        return comparator<Proj, Comp>{std::move(proj), std::move(comp), false};
    }
} compare_by{};

// ============================================================================
// Multi-level comparator - supports multiple sort keys
// ============================================================================

template <typename... Comps> struct multi_comparator {
    std::tuple<Comps...> comps;

    constexpr explicit multi_comparator(Comps... cs) : comps(std::move(cs)...) {}

    constexpr explicit multi_comparator(std::tuple<Comps...> t) : comps(std::move(t)) {}

    template <typename T> constexpr bool operator()(const T &a, const T &b) const {
        return compare_impl(a, b, std::index_sequence_for<Comps...>{});
    }

  private:
    template <typename T, std::size_t... Is>
    constexpr bool compare_impl(const T &a, const T &b, std::index_sequence<Is...>) const {
        bool result = false;
        bool decided = false;
        ((decided || (std::get<Is>(comps)(a, b)   ? (result = true, decided = true)
                      : std::get<Is>(comps)(b, a) ? (decided = true)
                                                  : false)),
         ...);
        return result;
    }
};

template <typename... Comps> multi_comparator(Comps...) -> multi_comparator<Comps...>;

// CPO: rpp::multi_compare
template <typename... Comps> [[nodiscard]] constexpr auto multi_compare(Comps... comps) {
    return multi_comparator<Comps...>{std::move(comps)...};
}

// ============================================================================
// Natural sort comparator (numeric-aware sorting)
// ============================================================================

struct natural_comparator {
    template <typename T>
        requires std::convertible_to<T, std::string_view>
    bool operator()(const T &a, const T &b) const {
        return natural_compare(std::string_view(a), std::string_view(b));
    }

    static bool natural_compare(std::string_view a, std::string_view b) {
        auto ai = a.begin(), ae = a.end();
        auto bi = b.begin(), be = b.end();

        while (ai != ae && bi != be) {
            if (std::isdigit(static_cast<unsigned char>(*ai)) && std::isdigit(static_cast<unsigned char>(*bi))) {
                // Compare numeric parts
                auto a_start = ai;
                auto b_start = bi;

                while (ai != ae && std::isdigit(static_cast<unsigned char>(*ai))) {
                    ++ai;
                }
                while (bi != be && std::isdigit(static_cast<unsigned char>(*bi))) {
                    ++bi;
                }

                auto a_len = ai - a_start;
                auto b_len = bi - b_start;

                // Skip leading zeros for comparison
                while (a_len > 1 && *a_start == '0') {
                    ++a_start;
                    --a_len;
                }
                while (b_len > 1 && *b_start == '0') {
                    ++b_start;
                    --b_len;
                }

                if (a_len != b_len) {
                    return a_len < b_len;
                }

                auto cmp = std::string_view(&*a_start, a_len).compare(std::string_view(&*b_start, b_len));
                if (cmp != 0) {
                    return cmp < 0;
                }
            } else {
                // Compare character by character (case-insensitive)
                auto ca = std::tolower(static_cast<unsigned char>(*ai));
                auto cb = std::tolower(static_cast<unsigned char>(*bi));
                if (ca != cb) {
                    return ca < cb;
                }
                ++ai;
                ++bi;
            }
        }

        return ai == ae && bi != be;
    }
};

template <typename Proj> struct natural_comparator_by {
    Proj proj;

    constexpr explicit natural_comparator_by(Proj p) : proj(std::move(p)) {}

    template <typename T> bool operator()(const T &a, const T &b) const {
        return natural_comparator{}(std::invoke(proj, a), std::invoke(proj, b));
    }

    [[nodiscard]] auto desc() const {
        return [p = proj](const auto &a, const auto &b) {
            return natural_comparator{}(std::invoke(p, b), std::invoke(p, a));
        };
    }
};

template <typename Proj> natural_comparator_by(Proj) -> natural_comparator_by<Proj>;

// CPO: rpp::natural_compare
inline constexpr struct natural_compare_fn {
    [[nodiscard]] constexpr auto operator()() const { return natural_comparator{}; }

    template <typename Proj> [[nodiscard]] constexpr auto operator()(Proj proj) const {
        return natural_comparator_by<Proj>{std::move(proj)};
    }
} natural_compare{};

// ============================================================================
// Reverse comparator
// ============================================================================

template <typename Comp> struct reverse_comparator {
    Comp comp;

    template <typename T> constexpr bool operator()(const T &a, const T &b) const { return comp(b, a); }
};

template <typename Comp> [[nodiscard]] constexpr auto reverse(Comp comp) {
    return reverse_comparator<Comp>{std::move(comp)};
}

} // namespace rpp
