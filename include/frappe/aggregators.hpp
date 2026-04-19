#ifndef FRAPPE_AGGREGATORS_HPP
#define FRAPPE_AGGREGATORS_HPP

#include "frappe/entry.hpp"
#include "frappe/projections.hpp"
#include <ranges>
#include <vector>
#include <optional>
#include <map>
#include <unordered_map>
#include <set>
#include <numeric>
#include <random>

namespace frappe {

// ============================================================================
// Selection adapters
// ============================================================================

// Take first element (in entry namespace to avoid conflict with find.hpp)
namespace entry {
    struct first_adapter {
        template<std::ranges::input_range R>
        auto operator()(R&& r) const -> std::optional<std::ranges::range_value_t<R>> {
            auto it = std::ranges::begin(r);
            if (it != std::ranges::end(r)) {
                return *it;
            }
            return std::nullopt;
        }
        
        template<std::ranges::input_range R>
        friend auto operator|(R&& r, const first_adapter& f) {
            return f(std::forward<R>(r));
        }
    };

    inline constexpr first_adapter first{};
} // namespace entry

// Take last element
struct last_adapter {
    template<std::ranges::input_range R>
    auto operator()(R&& r) const -> std::optional<std::ranges::range_value_t<R>> {
        std::optional<std::ranges::range_value_t<R>> result;
        for (auto&& item : r) {
            result = std::forward<decltype(item)>(item);
        }
        return result;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const last_adapter& l) {
        return l(std::forward<R>(r));
    }
};

inline constexpr last_adapter last{};

// Take first N elements (in entry namespace to avoid conflict with find.hpp)
namespace entry {
    struct take_adapter {
        std::size_t count;
        
        template<std::ranges::input_range R>
        auto operator()(R&& r) const {
            std::vector<std::ranges::range_value_t<R>> result;
            result.reserve(count);
            std::size_t i = 0;
            for (auto&& item : r) {
                if (i++ >= count) break;
                result.push_back(std::forward<decltype(item)>(item));
            }
            return result;
        }
        
        template<std::ranges::input_range R>
        friend auto operator|(R&& r, const take_adapter& t) {
            return t(std::forward<R>(r));
        }
    };

    [[nodiscard]] inline take_adapter take(std::size_t n) {
        return take_adapter{n};
    }
} // namespace entry

// Skip first N elements
struct skip_adapter {
    std::size_t count;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        std::vector<std::ranges::range_value_t<R>> result;
        std::size_t i = 0;
        for (auto&& item : r) {
            if (i++ >= count) {
                result.push_back(std::forward<decltype(item)>(item));
            }
        }
        return result;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const skip_adapter& s) {
        return s(std::forward<R>(r));
    }
};

[[nodiscard]] inline skip_adapter skip(std::size_t n) {
    return skip_adapter{n};
}

// Take while predicate is true
template<typename Pred>
struct take_while_adapter {
    Pred predicate;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        std::vector<std::ranges::range_value_t<R>> result;
        for (auto&& item : r) {
            if (!predicate(item)) break;
            result.push_back(std::forward<decltype(item)>(item));
        }
        return result;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const take_while_adapter& t) {
        return t(std::forward<R>(r));
    }
};

template<typename Pred>
[[nodiscard]] inline take_while_adapter<Pred> take_while(Pred pred) {
    return take_while_adapter<Pred>{std::move(pred)};
}

// Drop while predicate is true
template<typename Pred>
struct drop_while_adapter {
    Pred predicate;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        std::vector<std::ranges::range_value_t<R>> result;
        bool dropping = true;
        for (auto&& item : r) {
            if (dropping && predicate(item)) continue;
            dropping = false;
            result.push_back(std::forward<decltype(item)>(item));
        }
        return result;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const drop_while_adapter& d) {
        return d(std::forward<R>(r));
    }
};

template<typename Pred>
[[nodiscard]] inline drop_while_adapter<Pred> drop_while(Pred pred) {
    return drop_while_adapter<Pred>{std::move(pred)};
}

// ============================================================================
// Counting adapters
// ============================================================================

// Count elements
struct count_adapter {
    template<std::ranges::input_range R>
    auto operator()(R&& r) const -> std::size_t {
        std::size_t n = 0;
        for ([[maybe_unused]] auto&& item : r) {
            ++n;
        }
        return n;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const count_adapter& c) {
        return c(std::forward<R>(r));
    }
};

inline constexpr count_adapter count{};

// Count with predicate
template<typename Pred>
struct count_if_adapter {
    Pred predicate;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const -> std::size_t {
        std::size_t n = 0;
        for (auto&& item : r) {
            if (predicate(item)) ++n;
        }
        return n;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const count_if_adapter& c) {
        return c(std::forward<R>(r));
    }
};

template<typename Pred>
[[nodiscard]] inline count_if_adapter<Pred> count_if(Pred pred) {
    return count_if_adapter<Pred>{std::move(pred)};
}

// ============================================================================
// Size calculation adapters
// ============================================================================

// Total size of all files (in entry namespace to avoid conflict with find.hpp)
namespace entry {
    struct total_size_adapter {
        template<std::ranges::input_range R>
        auto operator()(R&& r) const -> std::uintmax_t {
            std::uintmax_t total = 0;
            for (const auto& item : r) {
                if constexpr (std::is_same_v<std::decay_t<decltype(item)>, file_entry>) {
                    total += item.size;
                } else if constexpr (std::is_same_v<std::decay_t<decltype(item)>, path>) {
                    std::error_code ec;
                    if (std::filesystem::is_regular_file(item, ec)) {
                        total += std::filesystem::file_size(item, ec);
                    }
                }
            }
            return total;
        }
        
        template<std::ranges::input_range R>
        friend auto operator|(R&& r, const total_size_adapter& t) {
            return t(std::forward<R>(r));
        }
    };

    inline constexpr total_size_adapter total_size{};
} // namespace entry

// Average size
struct avg_size_adapter {
    template<std::ranges::input_range R>
    auto operator()(R&& r) const -> double {
        std::uintmax_t total = 0;
        std::size_t count = 0;
        for (const auto& item : r) {
            if constexpr (std::is_same_v<std::decay_t<decltype(item)>, file_entry>) {
                total += item.size;
                ++count;
            }
        }
        return count > 0 ? static_cast<double>(total) / count : 0.0;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const avg_size_adapter& a) {
        return a(std::forward<R>(r));
    }
};

inline constexpr avg_size_adapter avg_size{};

// ============================================================================
// Grouping adapters
// ============================================================================

// Group by projection
template<typename Proj>
struct group_by_adapter {
    Proj projection;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        using Key = std::decay_t<std::invoke_result_t<Proj, std::ranges::range_value_t<R>>>;
        using Value = std::ranges::range_value_t<R>;
        
        std::map<Key, std::vector<Value>> groups;
        for (auto&& item : r) {
            groups[projection(item)].push_back(std::forward<decltype(item)>(item));
        }
        return groups;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const group_by_adapter& g) {
        return g(std::forward<R>(r));
    }
};

template<typename Proj>
[[nodiscard]] inline group_by_adapter<Proj> group_by(Proj proj) {
    return group_by_adapter<Proj>{std::move(proj)};
}

// Count by projection
template<typename Proj>
struct count_by_adapter {
    Proj projection;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        using Key = std::decay_t<std::invoke_result_t<Proj, std::ranges::range_value_t<R>>>;
        
        std::map<Key, std::size_t> counts;
        for (const auto& item : r) {
            ++counts[projection(item)];
        }
        return counts;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const count_by_adapter& c) {
        return c(std::forward<R>(r));
    }
};

template<typename Proj>
[[nodiscard]] inline count_by_adapter<Proj> count_by(Proj proj) {
    return count_by_adapter<Proj>{std::move(proj)};
}

// Sum by projection (e.g., total size per extension)
template<typename KeyProj, typename ValueProj>
struct sum_by_adapter {
    KeyProj key_projection;
    ValueProj value_projection;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        using Key = std::decay_t<std::invoke_result_t<KeyProj, std::ranges::range_value_t<R>>>;
        using Value = std::decay_t<std::invoke_result_t<ValueProj, std::ranges::range_value_t<R>>>;
        
        std::map<Key, Value> sums;
        for (const auto& item : r) {
            sums[key_projection(item)] += value_projection(item);
        }
        return sums;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const sum_by_adapter& s) {
        return s(std::forward<R>(r));
    }
};

template<typename KeyProj, typename ValueProj>
[[nodiscard]] inline sum_by_adapter<KeyProj, ValueProj> sum_by(KeyProj key_proj, ValueProj value_proj) {
    return sum_by_adapter<KeyProj, ValueProj>{std::move(key_proj), std::move(value_proj)};
}

// ============================================================================
// Min/Max adapters
// ============================================================================

template<typename Proj>
struct min_by_adapter {
    Proj projection;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const -> std::optional<std::ranges::range_value_t<R>> {
        std::optional<std::ranges::range_value_t<R>> result;
        for (auto&& item : r) {
            if (!result || projection(item) < projection(*result)) {
                result = std::forward<decltype(item)>(item);
            }
        }
        return result;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const min_by_adapter& m) {
        return m(std::forward<R>(r));
    }
};

template<typename Proj>
[[nodiscard]] inline min_by_adapter<Proj> min_by(Proj proj) {
    return min_by_adapter<Proj>{std::move(proj)};
}

template<typename Proj>
struct max_by_adapter {
    Proj projection;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const -> std::optional<std::ranges::range_value_t<R>> {
        std::optional<std::ranges::range_value_t<R>> result;
        for (auto&& item : r) {
            if (!result || projection(item) > projection(*result)) {
                result = std::forward<decltype(item)>(item);
            }
        }
        return result;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const max_by_adapter& m) {
        return m(std::forward<R>(r));
    }
};

template<typename Proj>
[[nodiscard]] inline max_by_adapter<Proj> max_by(Proj proj) {
    return max_by_adapter<Proj>{std::move(proj)};
}

// Convenience: min/max by size
inline const auto smallest = min_by(proj::size);
inline const auto largest = max_by(proj::size);

// Convenience: min/max by mtime
inline const auto oldest = min_by(proj::mtime);
inline const auto newest = max_by(proj::mtime);

// ============================================================================
// Collect adapter (materialize range to vector)
// ============================================================================

struct collect_adapter {
    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        std::vector<std::ranges::range_value_t<R>> result;
        for (auto&& item : r) {
            result.push_back(std::forward<decltype(item)>(item));
        }
        return result;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const collect_adapter& c) {
        return c(std::forward<R>(r));
    }
};

inline constexpr collect_adapter collect{};

// ============================================================================
// Any/All/None adapters
// ============================================================================

template<typename Pred>
struct any_adapter {
    Pred predicate;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const -> bool {
        for (const auto& item : r) {
            if (predicate(item)) return true;
        }
        return false;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const any_adapter& a) {
        return a(std::forward<R>(r));
    }
};

template<typename Pred>
[[nodiscard]] inline any_adapter<Pred> any_of(Pred pred) {
    return any_adapter<Pred>{std::move(pred)};
}

template<typename Pred>
struct all_adapter {
    Pred predicate;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const -> bool {
        for (const auto& item : r) {
            if (!predicate(item)) return false;
        }
        return true;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const all_adapter& a) {
        return a(std::forward<R>(r));
    }
};

template<typename Pred>
[[nodiscard]] inline all_adapter<Pred> all_of(Pred pred) {
    return all_adapter<Pred>{std::move(pred)};
}

template<typename Pred>
struct none_adapter {
    Pred predicate;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const -> bool {
        for (const auto& item : r) {
            if (predicate(item)) return false;
        }
        return true;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const none_adapter& n) {
        return n(std::forward<R>(r));
    }
};

template<typename Pred>
[[nodiscard]] inline none_adapter<Pred> none_of(Pred pred) {
    return none_adapter<Pred>{std::move(pred)};
}

// ============================================================================
// Partition adapter - split into two groups by predicate
// ============================================================================

template<typename Pred>
struct partition_adapter {
    Pred predicate;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        using T = std::ranges::range_value_t<R>;
        std::vector<T> matching;
        std::vector<T> not_matching;
        
        for (auto&& item : r) {
            if (predicate(item)) {
                matching.push_back(std::forward<decltype(item)>(item));
            } else {
                not_matching.push_back(std::forward<decltype(item)>(item));
            }
        }
        return std::make_pair(std::move(matching), std::move(not_matching));
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const partition_adapter& p) {
        return p(std::forward<R>(r));
    }
};

template<typename Pred>
[[nodiscard]] inline partition_adapter<Pred> partition_by(Pred pred) {
    return partition_adapter<Pred>{std::move(pred)};
}

// ============================================================================
// Unique adapter - remove consecutive duplicates by projection
// ============================================================================

template<typename Proj>
struct unique_by_adapter {
    Proj projection;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        std::vector<std::ranges::range_value_t<R>> result;
        std::optional<std::decay_t<std::invoke_result_t<Proj, std::ranges::range_value_t<R>>>> last_key;
        
        for (auto&& item : r) {
            auto key = projection(item);
            if (!last_key || *last_key != key) {
                result.push_back(std::forward<decltype(item)>(item));
                last_key = std::move(key);
            }
        }
        return result;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const unique_by_adapter& u) {
        return u(std::forward<R>(r));
    }
};

template<typename Proj>
[[nodiscard]] inline unique_by_adapter<Proj> unique_by(Proj proj) {
    return unique_by_adapter<Proj>{std::move(proj)};
}

// Unique by identity
struct unique_adapter {
    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        std::vector<std::ranges::range_value_t<R>> result;
        for (auto&& item : r) {
            if (result.empty() || !(result.back() == item)) {
                result.push_back(std::forward<decltype(item)>(item));
            }
        }
        return result;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const unique_adapter& u) {
        return u(std::forward<R>(r));
    }
};

inline constexpr unique_adapter unique{};

// ============================================================================
// Chunk adapter - split into fixed-size chunks
// ============================================================================

struct chunk_adapter {
    std::size_t chunk_size;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        using T = std::ranges::range_value_t<R>;
        std::vector<std::vector<T>> chunks;
        std::vector<T> current_chunk;
        current_chunk.reserve(chunk_size);
        
        for (auto&& item : r) {
            current_chunk.push_back(std::forward<decltype(item)>(item));
            if (current_chunk.size() >= chunk_size) {
                chunks.push_back(std::move(current_chunk));
                current_chunk = std::vector<T>();
                current_chunk.reserve(chunk_size);
            }
        }
        if (!current_chunk.empty()) {
            chunks.push_back(std::move(current_chunk));
        }
        return chunks;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const chunk_adapter& c) {
        return c(std::forward<R>(r));
    }
};

[[nodiscard]] inline chunk_adapter chunk(std::size_t n) {
    return chunk_adapter{n};
}

// ============================================================================
// Enumerate adapter - add index to each element
// ============================================================================

struct enumerate_adapter {
    std::size_t start = 0;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        using T = std::ranges::range_value_t<R>;
        std::vector<std::pair<std::size_t, T>> result;
        std::size_t idx = start;
        for (auto&& item : r) {
            result.emplace_back(idx++, std::forward<decltype(item)>(item));
        }
        return result;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const enumerate_adapter& e) {
        return e(std::forward<R>(r));
    }
};

inline constexpr enumerate_adapter enumerate{};

[[nodiscard]] inline enumerate_adapter enumerate_from(std::size_t start) {
    return enumerate_adapter{start};
}

// ============================================================================
// Nth element adapter
// ============================================================================

struct nth_adapter {
    std::size_t index;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const -> std::optional<std::ranges::range_value_t<R>> {
        std::size_t i = 0;
        for (auto&& item : r) {
            if (i++ == index) {
                return std::forward<decltype(item)>(item);
            }
        }
        return std::nullopt;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const nth_adapter& n) {
        return n(std::forward<R>(r));
    }
};

[[nodiscard]] inline nth_adapter nth(std::size_t n) {
    return nth_adapter{n};
}

// ============================================================================
// Sample adapter - random sample of n elements
// ============================================================================

struct sample_adapter {
    std::size_t count;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        std::vector<std::ranges::range_value_t<R>> result;
        for (auto&& item : r) {
            result.push_back(std::forward<decltype(item)>(item));
        }
        
        if (result.size() <= count) {
            return result;
        }
        
        // Fisher-Yates shuffle for first 'count' elements
        std::random_device rd;
        std::mt19937 gen(rd());
        for (std::size_t i = 0; i < count; ++i) {
            std::uniform_int_distribution<std::size_t> dist(i, result.size() - 1);
            std::swap(result[i], result[dist(gen)]);
        }
        result.resize(count);
        return result;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const sample_adapter& s) {
        return s(std::forward<R>(r));
    }
};

[[nodiscard]] inline sample_adapter sample(std::size_t n) {
    return sample_adapter{n};
}

// ============================================================================
// Flatten adapter - flatten nested ranges
// ============================================================================

struct flatten_adapter {
    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        using Inner = std::ranges::range_value_t<R>;
        using T = std::ranges::range_value_t<Inner>;
        std::vector<T> result;
        for (auto&& inner : r) {
            for (auto&& item : inner) {
                result.push_back(std::forward<decltype(item)>(item));
            }
        }
        return result;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const flatten_adapter& f) {
        return f(std::forward<R>(r));
    }
};

inline constexpr flatten_adapter flatten{};

// ============================================================================
// Intersperse adapter - insert element between each pair
// ============================================================================

template<typename T>
struct intersperse_adapter {
    T separator;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        std::vector<std::ranges::range_value_t<R>> result;
        bool first = true;
        for (auto&& item : r) {
            if (!first) {
                result.push_back(separator);
            }
            first = false;
            result.push_back(std::forward<decltype(item)>(item));
        }
        return result;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const intersperse_adapter& i) {
        return i(std::forward<R>(r));
    }
};

template<typename T>
[[nodiscard]] inline intersperse_adapter<T> intersperse(T sep) {
    return intersperse_adapter<T>{std::move(sep)};
}

// ============================================================================
// Dedup adapter - remove all duplicates (not just consecutive)
// ============================================================================

template<typename Proj>
struct dedup_by_adapter {
    Proj projection;
    
    template<std::ranges::input_range R>
    auto operator()(R&& r) const {
        using T = std::ranges::range_value_t<R>;
        using Key = std::decay_t<std::invoke_result_t<Proj, T>>;
        
        std::vector<T> result;
        std::set<Key> seen;
        
        for (auto&& item : r) {
            auto key = projection(item);
            if (seen.insert(key).second) {
                result.push_back(std::forward<decltype(item)>(item));
            }
        }
        return result;
    }
    
    template<std::ranges::input_range R>
    friend auto operator|(R&& r, const dedup_by_adapter& d) {
        return d(std::forward<R>(r));
    }
};

template<typename Proj>
[[nodiscard]] inline dedup_by_adapter<Proj> dedup_by(Proj proj) {
    return dedup_by_adapter<Proj>{std::move(proj)};
}

// Dedup by name (common use case for file entries)
inline const auto dedup_by_name = dedup_by(proj::name);
inline const auto dedup_by_inode = dedup_by(proj::inode);

} // namespace frappe

#endif // FRAPPE_AGGREGATORS_HPP
