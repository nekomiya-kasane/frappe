#ifndef FRAPPE_TRAVERSAL_HPP
#define FRAPPE_TRAVERSAL_HPP

#include "frappe/exports.hpp"
#include "frappe/entry.hpp"
#include "frappe/status.hpp"
#include <filesystem>
#include <functional>
#include <memory>
#include <chrono>
#include <unordered_set>

namespace frappe {

using path = std::filesystem::path;

// ============================================================================
// Cycle detection strategy
// ============================================================================

enum class cycle_detection {
    none,           // No detection (dangerous with follow_symlinks!)
    inode_set,      // Use inode set (precise but memory-intensive)
    path_set,       // Use canonical path set
    depth_limit,    // Only use depth limit
    hybrid          // Hybrid strategy (recommended)
};

// ============================================================================
// Action when cycle is detected
// ============================================================================

enum class on_cycle {
    skip,           // Skip the cyclic directory
    error,          // Report error and stop
    callback        // Call user callback
};

// ============================================================================
// Traversal options
// ============================================================================

struct traversal_options {
    bool follow_symlinks = false;           // Follow symbolic links
    bool skip_permission_denied = true;     // Skip directories with permission denied
    bool include_hidden = false;            // Include hidden files
    int max_depth = -1;                     // Maximum depth (-1 = unlimited)
    bool cross_device = true;               // Cross filesystem boundaries
    std::size_t max_entries = 0;            // Maximum entries (0 = unlimited)
    
    // Cycle detection
    cycle_detection cycle_strategy = cycle_detection::hybrid;
    on_cycle cycle_action = on_cycle::skip;
    std::function<void(const path& link, const path& target)> cycle_callback;
    
    // Error callback
    std::function<void(const path& p, std::error_code ec)> error_callback;
    
    // Builder pattern
    traversal_options& with_follow_symlinks(bool v = true) { follow_symlinks = v; return *this; }
    traversal_options& with_skip_permission_denied(bool v = true) { skip_permission_denied = v; return *this; }
    traversal_options& with_include_hidden(bool v = true) { include_hidden = v; return *this; }
    traversal_options& with_max_depth(int d) { max_depth = d; return *this; }
    traversal_options& with_cross_device(bool v = true) { cross_device = v; return *this; }
    traversal_options& with_max_entries(std::size_t n) { max_entries = n; return *this; }
    traversal_options& with_cycle_detection(cycle_detection s) { cycle_strategy = s; return *this; }
    traversal_options& with_on_cycle(on_cycle a) { cycle_action = a; return *this; }
    traversal_options& with_cycle_callback(std::function<void(const path&, const path&)> cb) {
        cycle_callback = std::move(cb);
        cycle_action = on_cycle::callback;
        return *this;
    }
    traversal_options& with_error_callback(std::function<void(const path&, std::error_code)> cb) {
        error_callback = std::move(cb);
        return *this;
    }
};

// ============================================================================
// Traversal statistics
// ============================================================================

struct traversal_stats {
    std::size_t files_visited = 0;
    std::size_t dirs_visited = 0;
    std::size_t symlinks_visited = 0;
    std::size_t cycles_detected = 0;
    std::size_t permission_denied = 0;
    std::size_t errors = 0;
    std::size_t max_depth_reached = 0;
    std::chrono::nanoseconds elapsed{};
};

// ============================================================================
// Cycle detector
// ============================================================================

class FRAPPE_API cycle_detector {
public:
    explicit cycle_detector(cycle_detection strategy = cycle_detection::hybrid);
    
    // Check if entering directory would create a cycle
    // Returns true if safe, false if cycle detected
    [[nodiscard]] bool check_and_enter(const path& dir, const file_id& id);
    
    // Leave directory (for ancestor stack tracking)
    void leave(const path& dir);
    
    // Clear all state
    void clear();
    
    // Get detected cycle target (valid after check_and_enter returns false)
    [[nodiscard]] const path& cycle_target() const noexcept;

private:
    cycle_detection _strategy;
    
    // For inode_set strategy
    std::unordered_set<std::uint64_t> _visited_inodes;
    
    // For path_set strategy
    std::unordered_set<std::string> _visited_paths;
    
    // For hybrid strategy: ancestor stack
    struct ancestor_entry {
        path dir_path;
        std::uint64_t inode;
    };
    std::vector<ancestor_entry> _ancestor_stack;
    
    path _cycle_target;
};

// ============================================================================
// Directory traverser
// ============================================================================

class FRAPPE_API directory_traverser {
public:
    explicit directory_traverser(const path& root, const traversal_options& opts = {});
    ~directory_traverser();
    
    // Non-copyable, movable
    directory_traverser(const directory_traverser&) = delete;
    directory_traverser& operator=(const directory_traverser&) = delete;
    directory_traverser(directory_traverser&&) noexcept;
    directory_traverser& operator=(directory_traverser&&) noexcept;
    
    // Iterator interface
    class iterator;
    [[nodiscard]] iterator begin();
    [[nodiscard]] iterator end();
    
#ifdef FRAPPE_HAS_GENERATOR
    // Generator interface (C++23)
    [[nodiscard]] std::generator<file_entry> entries();
#endif
    
    // Get statistics
    [[nodiscard]] const traversal_stats& stats() const noexcept;
    
    // Cancel traversal
    void cancel() noexcept;
    [[nodiscard]] bool is_cancelled() const noexcept;

private:
    struct impl;
    std::unique_ptr<impl> _impl;
};

// ============================================================================
// Iterator for directory_traverser
// ============================================================================

class FRAPPE_API directory_traverser::iterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = file_entry;
    using difference_type = std::ptrdiff_t;
    using pointer = const file_entry*;
    using reference = const file_entry&;
    
    iterator() = default;
    
    reference operator*() const;
    pointer operator->() const;
    
    iterator& operator++();
    iterator operator++(int);
    
    bool operator==(const iterator& other) const;
    bool operator!=(const iterator& other) const { return !(*this == other); }

private:
    friend class directory_traverser;
    struct impl;
    std::shared_ptr<impl> _impl;
    
    explicit iterator(std::shared_ptr<impl> impl);
};

// ============================================================================
// Convenience functions
// ============================================================================

#ifdef FRAPPE_HAS_GENERATOR
// Traverse directory and yield entries
[[nodiscard]] std::generator<file_entry> traverse(
    const path& root,
    const traversal_options& opts = {}
);
#endif

// Traverse and collect all entries
[[nodiscard]] FRAPPE_API std::vector<file_entry> traverse_collect(
    const path& root,
    const traversal_options& opts = {}
);

// Traverse with callback
FRAPPE_API void traverse_with(
    const path& root,
    const std::function<bool(const file_entry&)>& callback,
    const traversal_options& opts = {}
);

// Parallel traversal (returns collected entries)
[[nodiscard]] FRAPPE_API std::vector<file_entry> traverse_parallel(
    const path& root,
    const traversal_options& opts = {},
    unsigned int threads = 0  // 0 = hardware_concurrency
);

// ============================================================================
// Safe recursive iteration (handles cycles automatically)
// ============================================================================

class FRAPPE_API safe_recursive_iterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::filesystem::directory_entry;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
    
    safe_recursive_iterator() = default;
    explicit safe_recursive_iterator(const path& root, const traversal_options& opts = {});
    
    reference operator*() const;
    pointer operator->() const;
    
    safe_recursive_iterator& operator++();
    safe_recursive_iterator operator++(int);
    
    bool operator==(const safe_recursive_iterator& other) const;
    bool operator!=(const safe_recursive_iterator& other) const { return !(*this == other); }
    
    // Get current depth
    [[nodiscard]] int depth() const noexcept;
    
    // Skip current directory's remaining entries
    void pop();
    
    // Disable recursion into current directory
    void disable_recursion_pending();

private:
    struct impl;
    std::shared_ptr<impl> _impl;
};

// Range wrapper for safe_recursive_iterator
class FRAPPE_API safe_recursive_directory_range {
public:
    safe_recursive_directory_range(const path& root, const traversal_options& opts = {});
    
    [[nodiscard]] safe_recursive_iterator begin() const;
    [[nodiscard]] safe_recursive_iterator end() const;

private:
    path _root;
    traversal_options _opts;
};

// Convenience function
[[nodiscard]] inline safe_recursive_directory_range safe_recursive_directory(
    const path& root,
    const traversal_options& opts = {}
) {
    return safe_recursive_directory_range(root, opts);
}

} // namespace frappe

#endif // FRAPPE_TRAVERSAL_HPP
