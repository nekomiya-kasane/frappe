#pragma once

#include "aggregate.hpp"

#include <cmath>
#include <numeric>
#include <range/v3/algorithm/all_of.hpp>
#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/algorithm/max_element.hpp>
#include <range/v3/algorithm/min_element.hpp>
#include <range/v3/algorithm/none_of.hpp>

namespace rpp {

    // ============================================================================
    // Average
    // ============================================================================

    template <typename Proj = std::identity> struct avg_view : pipeable_base<avg_view<Proj>> {
        Proj proj;

        constexpr explicit avg_view(Proj p = {}) : proj(std::move(p)) {}

        template <input_range R> constexpr auto operator()(R &&r) const {
            using T = std::invoke_result_t<Proj, range_value_t<R>>;
            T sum{};
            std::size_t count = 0;
            for (const auto &item : r) {
                sum += std::invoke(proj, item);
                ++count;
            }
            if constexpr (std::is_integral_v<T>) {
                return count > 0 ? static_cast<double>(sum) / static_cast<double>(count) : 0.0;
            } else {
                return count > 0 ? sum / static_cast<T>(count) : T{};
            }
        }
    };

    inline constexpr struct avg_fn {
        [[nodiscard]] constexpr auto operator()() const { return avg_view<>{}; }

        template <typename Proj> [[nodiscard]] constexpr auto operator()(Proj proj) const {
            return avg_view<Proj>{std::move(proj)};
        }
    } avg{};

    // ============================================================================
    // Max/Min by projection (returns element, not projected value)
    // ============================================================================

    template <typename Proj = std::identity> struct max_by_view : pipeable_base<max_by_view<Proj>> {
        Proj proj;

        constexpr explicit max_by_view(Proj p = {}) : proj(std::move(p)) {}

        template <input_range R> constexpr auto operator()(R &&r) const -> std::optional<range_value_t<R>> {
            auto it = ::ranges::max_element(r, std::less<>{}, proj);
            if (it != ::ranges::end(r)) {
                return *it;
            }
            return std::nullopt;
        }
    };

    inline constexpr struct max_by_fn {
        [[nodiscard]] constexpr auto operator()() const { return max_by_view<>{}; }

        template <typename Proj> [[nodiscard]] constexpr auto operator()(Proj proj) const {
            return max_by_view<Proj>{std::move(proj)};
        }
    } max_by{};

    template <typename Proj = std::identity> struct min_by_view : pipeable_base<min_by_view<Proj>> {
        Proj proj;

        constexpr explicit min_by_view(Proj p = {}) : proj(std::move(p)) {}

        template <input_range R> constexpr auto operator()(R &&r) const -> std::optional<range_value_t<R>> {
            auto it = ::ranges::min_element(r, std::less<>{}, proj);
            if (it != ::ranges::end(r)) {
                return *it;
            }
            return std::nullopt;
        }
    };

    inline constexpr struct min_by_fn {
        [[nodiscard]] constexpr auto operator()() const { return min_by_view<>{}; }

        template <typename Proj> [[nodiscard]] constexpr auto operator()(Proj proj) const {
            return min_by_view<Proj>{std::move(proj)};
        }
    } min_by{};

    // ============================================================================
    // Existence checks
    // ============================================================================

    inline constexpr struct any_fn : pipeable_base<any_fn> {
        template <input_range R> constexpr bool operator()(R &&r) const {
            return ::ranges::begin(r) != ::ranges::end(r);
        }
    } any{};

    inline constexpr struct empty_fn : pipeable_base<empty_fn> {
        template <input_range R> constexpr bool operator()(R &&r) const {
            return ::ranges::begin(r) == ::ranges::end(r);
        }
    } empty{};

    inline constexpr struct exists_fn {
        template <typename Pred> [[nodiscard]] constexpr auto operator()(Pred pred) const {
            return make_closure(
                [p = std::move(pred)](auto &&r) { return ::ranges::any_of(std::forward<decltype(r)>(r), p); });
        }
    } exists{};

    inline constexpr struct all_fn {
        template <typename Pred> [[nodiscard]] constexpr auto operator()(Pred pred) const {
            return make_closure(
                [p = std::move(pred)](auto &&r) { return ::ranges::all_of(std::forward<decltype(r)>(r), p); });
        }
    } all{};

    inline constexpr struct none_fn {
        template <typename Pred> [[nodiscard]] constexpr auto operator()(Pred pred) const {
            return make_closure(
                [p = std::move(pred)](auto &&r) { return ::ranges::none_of(std::forward<decltype(r)>(r), p); });
        }
    } none{};

    // ============================================================================
    // Statistical functions
    // ============================================================================

    template <typename Proj = std::identity> struct variance_view : pipeable_base<variance_view<Proj>> {
        Proj proj;
        bool sample = false; // true for sample variance, false for population variance

        constexpr variance_view(Proj p = {}, bool s = false) : proj(std::move(p)), sample(s) {}

        template <input_range R> constexpr double operator()(R &&r) const {
            double sum = 0;
            double sum_sq = 0;
            std::size_t count = 0;

            for (const auto &item : r) {
                double v = static_cast<double>(std::invoke(proj, item));
                sum += v;
                sum_sq += v * v;
                ++count;
            }

            if (count == 0 || (sample && count == 1)) {
                return 0.0;
            }

            double mean = sum / count;
            double divisor = sample ? (count - 1) : count;
            return (sum_sq - count * mean * mean) / divisor;
        }
    };

    inline constexpr struct variance_fn {
        [[nodiscard]] constexpr auto operator()(bool sample = false) const {
            return variance_view<>{std::identity{}, sample};
        }

        template <typename Proj> [[nodiscard]] constexpr auto operator()(Proj proj, bool sample = false) const {
            return variance_view<Proj>{std::move(proj), sample};
        }
    } variance{};

    template <typename Proj = std::identity> struct stddev_view : pipeable_base<stddev_view<Proj>> {
        Proj proj;
        bool sample = false;

        constexpr stddev_view(Proj p = {}, bool s = false) : proj(std::move(p)), sample(s) {}

        template <input_range R> constexpr double operator()(R &&r) const {
            return std::sqrt(variance_view<Proj>{proj, sample}(std::forward<R>(r)));
        }
    };

    inline constexpr struct stddev_fn {
        [[nodiscard]] constexpr auto operator()(bool sample = false) const {
            return stddev_view<>{std::identity{}, sample};
        }

        template <typename Proj> [[nodiscard]] constexpr auto operator()(Proj proj, bool sample = false) const {
            return stddev_view<Proj>{std::move(proj), sample};
        }
    } stddev{};

    // ============================================================================
    // Median
    // ============================================================================

    template <typename Proj = std::identity> struct median_view : pipeable_base<median_view<Proj>> {
        Proj proj;

        constexpr explicit median_view(Proj p = {}) : proj(std::move(p)) {}

        template <input_range R> constexpr auto operator()(R &&r) const {
            using T = std::decay_t<std::invoke_result_t<Proj, range_value_t<R>>>;
            std::vector<T> values;
            for (const auto &item : r) {
                values.push_back(std::invoke(proj, item));
            }

            if (values.empty()) {
                if constexpr (std::is_arithmetic_v<T>) {
                    return static_cast<double>(T{});
                } else {
                    return T{};
                }
            }

            std::sort(values.begin(), values.end());
            auto n = values.size();

            if constexpr (std::is_arithmetic_v<T>) {
                if (n % 2 == 0) {
                    return (static_cast<double>(values[n / 2 - 1]) + static_cast<double>(values[n / 2])) / 2.0;
                } else {
                    return static_cast<double>(values[n / 2]);
                }
            } else {
                return values[n / 2];
            }
        }
    };

    inline constexpr struct median_fn {
        [[nodiscard]] constexpr auto operator()() const { return median_view<>{}; }

        template <typename Proj> [[nodiscard]] constexpr auto operator()(Proj proj) const {
            return median_view<Proj>{std::move(proj)};
        }
    } median{};

    // ============================================================================
    // Percentile
    // ============================================================================

    template <typename Proj = std::identity> struct percentile_view : pipeable_base<percentile_view<Proj>> {
        Proj proj;
        double p; // 0.0 to 1.0

        constexpr percentile_view(double percentile, Proj pr = {}) : proj(std::move(pr)), p(percentile) {}

        template <input_range R> constexpr auto operator()(R &&r) const {
            using T = std::decay_t<std::invoke_result_t<Proj, range_value_t<R>>>;
            std::vector<T> values;
            for (const auto &item : r) {
                values.push_back(std::invoke(proj, item));
            }

            if (values.empty()) {
                return T{};
            }

            std::sort(values.begin(), values.end());

            double idx = p * (values.size() - 1);
            std::size_t lower = static_cast<std::size_t>(idx);
            std::size_t upper = lower + 1;
            double frac = idx - lower;

            if (upper >= values.size()) {
                return values.back();
            }

            if constexpr (std::is_arithmetic_v<T>) {
                return static_cast<T>(values[lower] * (1 - frac) + values[upper] * frac);
            } else {
                return frac < 0.5 ? values[lower] : values[upper];
            }
        }
    };

    inline constexpr struct percentile_fn {
        [[nodiscard]] constexpr auto operator()(double p) const { return percentile_view<>{p}; }

        template <typename Proj> [[nodiscard]] constexpr auto operator()(double p, Proj proj) const {
            return percentile_view<Proj>{p, std::move(proj)};
        }
    } percentile{};

} // namespace rpp
