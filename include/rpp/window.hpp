#pragma once

#include "concepts.hpp"
#include "pipeable.hpp"

#include <map>
#include <optional>
#include <range/v3/range/conversion.hpp>
#include <vector>

namespace rpp {

// ============================================================================
// Window function context
// ============================================================================

template <typename T> struct window_context {
    std::size_t row_number;                            // ROW_NUMBER() - 1-indexed
    std::size_t rank;                                  // RANK()
    std::size_t dense_rank;                            // DENSE_RANK()
    const T *prev;                                     // LAG(1) - previous element
    const T *next;                                     // LEAD(1) - next element
    std::size_t partition_size;                        // COUNT() OVER partition
    std::size_t partition_index;                       // Index within partition (0-indexed)
    const std::vector<T> *partition_data;              // Full source data
    const std::vector<std::size_t> *partition_indices; // Sorted indices within partition
};

// ============================================================================
// Window function result
// ============================================================================

template <typename T, typename... Results> struct window_result {
    T value;
    std::tuple<Results...> window_values;
};

// ============================================================================
// Window function adapter
// ============================================================================

template <typename PartitionBy, typename OrderBy, typename... WindowFns>
struct window_view : pipeable_base<window_view<PartitionBy, OrderBy, WindowFns...>> {
    PartitionBy partition_by;
    OrderBy order_by;
    std::tuple<WindowFns...> window_fns;

    constexpr window_view(PartitionBy pb, OrderBy ob, WindowFns... wfs)
        : partition_by(std::move(pb)), order_by(std::move(ob)), window_fns(std::move(wfs)...) {}

    template <input_range R> constexpr auto operator()(R &&r) const {
        using T = range_value_t<R>;
        using K = std::decay_t<std::invoke_result_t<PartitionBy, T>>;

        // Collect all items
        auto items = std::forward<R>(r) | ::ranges::to<std::vector<T>>();

        // Group by partition key
        std::map<K, std::vector<std::size_t>> partitions;
        for (std::size_t i = 0; i < items.size(); ++i) {
            partitions[std::invoke(partition_by, items[i])].push_back(i);
        }

        // Sort each partition
        for (auto &[key, indices] : partitions) {
            std::sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
                return std::invoke(order_by, items[a]) < std::invoke(order_by, items[b]);
            });
        }

        // Build result with window function values
        using ResultTuple = std::tuple<std::invoke_result_t<WindowFns, window_context<T>, T>...>;
        std::vector<std::pair<T, ResultTuple>> result;
        result.reserve(items.size());

        for (auto &[key, indices] : partitions) {
            std::size_t partition_size = indices.size();

            // Calculate ranks
            std::vector<std::size_t> ranks(partition_size);
            std::vector<std::size_t> dense_ranks(partition_size);

            if (!indices.empty()) {
                ranks[0] = 1;
                dense_ranks[0] = 1;
                std::size_t current_rank = 1;
                std::size_t current_dense_rank = 1;

                for (std::size_t i = 1; i < partition_size; ++i) {
                    auto prev_val = std::invoke(order_by, items[indices[i - 1]]);
                    auto curr_val = std::invoke(order_by, items[indices[i]]);

                    if (curr_val != prev_val) {
                        current_rank = i + 1;
                        ++current_dense_rank;
                    }
                    ranks[i] = current_rank;
                    dense_ranks[i] = current_dense_rank;
                }
            }

            // Apply window functions
            for (std::size_t i = 0; i < partition_size; ++i) {
                std::size_t idx = indices[i];

                window_context<T> ctx{.row_number = i + 1,
                                      .rank = ranks[i],
                                      .dense_rank = dense_ranks[i],
                                      .prev = (i > 0) ? &items[indices[i - 1]] : nullptr,
                                      .next = (i + 1 < partition_size) ? &items[indices[i + 1]] : nullptr,
                                      .partition_size = partition_size,
                                      .partition_index = i,
                                      .partition_data = &items,
                                      .partition_indices = &indices};

                auto window_values = std::apply(
                    [&](const auto &...fns) { return std::make_tuple(fns(ctx, items[idx])...); }, window_fns);

                result.emplace_back(items[idx], std::move(window_values));
            }
        }

        return result;
    }
};

// ============================================================================
// Window function builder
// ============================================================================

template <typename PartitionBy, typename OrderBy> struct window_builder {
    PartitionBy partition_by;
    OrderBy order_by;

    template <typename... WindowFns> [[nodiscard]] constexpr auto select(WindowFns... fns) const {
        return window_view<PartitionBy, OrderBy, WindowFns...>{partition_by, order_by, std::move(fns)...};
    }
};

struct no_partition_fn {
    constexpr int operator()(const auto &) const { return 0; }
};

inline constexpr struct window_fn {
    template <typename PartitionBy, typename OrderBy>
    [[nodiscard]] constexpr auto operator()(PartitionBy partition_by, OrderBy order_by) const {
        return window_builder<PartitionBy, OrderBy>{std::move(partition_by), std::move(order_by)};
    }

    // No partition (entire range is one partition)
    template <typename OrderBy> [[nodiscard]] constexpr auto operator()(OrderBy order_by) const {
        return window_builder<no_partition_fn, OrderBy>{no_partition_fn{}, std::move(order_by)};
    }
} window{};

// ============================================================================
// Convenience window functions
// ============================================================================

namespace window_fns {

inline constexpr auto row_number = [](const auto &ctx, const auto &) { return ctx.row_number; };

inline constexpr auto rank = [](const auto &ctx, const auto &) { return ctx.rank; };

inline constexpr auto dense_rank = [](const auto &ctx, const auto &) { return ctx.dense_rank; };

inline constexpr auto partition_size = [](const auto &ctx, const auto &) { return ctx.partition_size; };

inline constexpr auto partition_index = [](const auto &ctx, const auto &) { return ctx.partition_index; };

template <typename Proj> constexpr auto lag(Proj proj, std::size_t offset = 1) {
    return [p = std::move(proj),
            offset](const auto &ctx,
                    const auto &) -> std::optional<std::invoke_result_t<Proj, std::decay_t<decltype(*ctx.prev)>>> {
        if (ctx.partition_index >= offset && ctx.partition_data && ctx.partition_indices) {
            auto target_idx = (*ctx.partition_indices)[ctx.partition_index - offset];
            return std::invoke(p, (*ctx.partition_data)[target_idx]);
        }
        return std::nullopt;
    };
}

template <typename Proj, typename Default> constexpr auto lag(Proj proj, std::size_t offset, Default default_value) {
    return [p = std::move(proj), offset, dv = std::move(default_value)](const auto &ctx, const auto &) {
        if (ctx.partition_index >= offset && ctx.partition_data && ctx.partition_indices) {
            auto target_idx = (*ctx.partition_indices)[ctx.partition_index - offset];
            return std::invoke(p, (*ctx.partition_data)[target_idx]);
        }
        return dv;
    };
}

template <typename Proj> constexpr auto lead(Proj proj, std::size_t offset = 1) {
    return [p = std::move(proj),
            offset](const auto &ctx,
                    const auto &) -> std::optional<std::invoke_result_t<Proj, std::decay_t<decltype(*ctx.next)>>> {
        if (ctx.partition_index + offset < ctx.partition_size && ctx.partition_data && ctx.partition_indices) {
            auto target_idx = (*ctx.partition_indices)[ctx.partition_index + offset];
            return std::invoke(p, (*ctx.partition_data)[target_idx]);
        }
        return std::nullopt;
    };
}

template <typename Proj, typename Default> constexpr auto lead(Proj proj, std::size_t offset, Default default_value) {
    return [p = std::move(proj), offset, dv = std::move(default_value)](const auto &ctx, const auto &) {
        if (ctx.partition_index + offset < ctx.partition_size && ctx.partition_data && ctx.partition_indices) {
            auto target_idx = (*ctx.partition_indices)[ctx.partition_index + offset];
            return std::invoke(p, (*ctx.partition_data)[target_idx]);
        }
        return dv;
    };
}

// NTILE - divide partition into n buckets
constexpr auto ntile(std::size_t n) {
    return [n](const auto &ctx, const auto &) { return (ctx.partition_index * n) / ctx.partition_size + 1; };
}

// PERCENT_RANK
inline constexpr auto percent_rank = [](const auto &ctx, const auto &) {
    if (ctx.partition_size <= 1) {
        return 0.0;
    }
    return static_cast<double>(ctx.rank - 1) / (ctx.partition_size - 1);
};

// CUME_DIST
inline constexpr auto cume_dist = [](const auto &ctx, const auto &) {
    return static_cast<double>(ctx.row_number) / ctx.partition_size;
};

} // namespace window_fns

// ============================================================================
// Simplified row number assignment
// ============================================================================

template <typename OrderBy = std::identity> struct with_row_number_view : pipeable_base<with_row_number_view<OrderBy>> {
    OrderBy order_by;

    constexpr explicit with_row_number_view(OrderBy ob = {}) : order_by(std::move(ob)) {}

    template <input_range R> constexpr auto operator()(R &&r) const {
        using T = range_value_t<R>;
        auto items = std::forward<R>(r) | ::ranges::to<std::vector<T>>();

        // Sort if order_by is not identity
        if constexpr (!std::is_same_v<OrderBy, std::identity>) {
            std::sort(items.begin(), items.end(),
                      [this](const T &a, const T &b) { return std::invoke(order_by, a) < std::invoke(order_by, b); });
        }

        std::vector<std::pair<std::size_t, T>> result;
        result.reserve(items.size());
        for (std::size_t i = 0; i < items.size(); ++i) {
            result.emplace_back(i + 1, std::move(items[i]));
        }
        return result;
    }
};

inline constexpr struct with_row_number_fn {
    [[nodiscard]] constexpr auto operator()() const { return with_row_number_view<>{}; }

    template <typename OrderBy> [[nodiscard]] constexpr auto operator()(OrderBy order_by) const {
        return with_row_number_view<OrderBy>{std::move(order_by)};
    }
} with_row_number{};

} // namespace rpp
