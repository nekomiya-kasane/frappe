#ifndef FRAPPE_WATCHER_HPP
#define FRAPPE_WATCHER_HPP

#include "frappe/error.hpp"
#include "frappe/exports.hpp"

#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace frappe {

using path = std::filesystem::path;
using file_time_type = std::filesystem::file_time_type;

// ============================================================================
// 9.1 Watch Event Types
// ============================================================================

/**
 * @brief Types of file system watch events
 */
enum class watch_event_type {
    added,      ///< File/directory created
    modified,   ///< File content modified
    removed,    ///< File/directory deleted
    renamed,    ///< Renamed (includes old and new names)
    attributes, ///< Attributes changed (permissions, timestamps, etc.)
    error       ///< Watch error
};

/**
 * @brief Get the string name of a watch event type
 * @param type The watch event type
 * @return String view of the event type name
 */
[[nodiscard]] FRAPPE_API std::string_view watch_event_type_name(watch_event_type type) noexcept;

// ============================================================================
// 9.2 Watch Event Structure
// ============================================================================

/**
 * @brief Represents a file system watch event
 */
struct watch_event {
    watch_event_type type = watch_event_type::error; ///< Event type
    path file_path;                                  ///< Path of the affected file/directory
    path old_path;                                   ///< Old path (only for renamed events)
    file_time_type timestamp{};                      ///< Timestamp of the event
    std::error_code error;                           ///< Error code (only for error events)
    bool is_directory = false;                       ///< True if the path is a directory
};

// ============================================================================
// 9.3 Watch Options
// ============================================================================

/**
 * @brief Configuration options for file watching
 */
struct watch_options {
    bool recursive = true;                        ///< Recursively watch subdirectories
    bool follow_symlinks = false;                 ///< Follow symbolic links
    bool ignore_initial = false;                  ///< Ignore initial scan events
    bool use_polling = false;                     ///< Force polling mode
    std::chrono::milliseconds poll_interval{100}; ///< Polling interval
    std::chrono::milliseconds debounce{50};       ///< Debounce delay
    std::chrono::milliseconds atomic_delay{100};  ///< Atomic write detection delay
    std::vector<std::string> ignored_patterns;    ///< Glob patterns to ignore
    std::size_t max_depth = 0;                    ///< Maximum depth (0 = unlimited)
};

// ============================================================================
// 9.4 File Watcher
// ============================================================================

/**
 * @brief File system watcher for monitoring file/directory changes
 *
 * This class provides functionality to watch for file system changes
 * such as file creation, modification, deletion, and renaming.
 * It supports both native OS notifications and polling mode.
 */
class FRAPPE_API file_watcher {
  public:
    using event_callback = std::function<void(const watch_event &)>;
    using error_callback = std::function<void(std::error_code)>;
    using filter_callback = std::function<bool(const path &)>;

    /**
     * @brief Construct a file watcher with specified options
     * @param options Watch configuration options
     */
    explicit file_watcher(watch_options options = {});

    /**
     * @brief Destructor
     */
    ~file_watcher();

    // Non-copyable
    file_watcher(const file_watcher &) = delete;
    file_watcher &operator=(const file_watcher &) = delete;

    // Movable
    file_watcher(file_watcher &&other) noexcept;
    file_watcher &operator=(file_watcher &&other) noexcept;

    /**
     * @brief Add a path to watch
     * @param p Path to watch
     * @return Result indicating success or error
     */
    [[nodiscard]] void_result add(const path &p);

    /**
     * @brief Add multiple paths to watch
     * @param paths Span of paths to watch
     * @return Result indicating success or error
     */
    [[nodiscard]] void_result add(std::span<const path> paths);

    /**
     * @brief Remove a path from watching
     * @param p Path to stop watching
     * @return Result indicating success or error
     */
    [[nodiscard]] void_result remove(const path &p);

    /**
     * @brief Remove all watched paths
     */
    void remove_all();

    /**
     * @brief Set callback for all events
     * @param callback Function to call on any event
     */
    void on_event(event_callback callback);

    /**
     * @brief Set callback for file/directory addition events
     * @param callback Function to call when files are added
     */
    void on_add(event_callback callback);

    /**
     * @brief Set callback for file modification events
     * @param callback Function to call when files are modified
     */
    void on_change(event_callback callback);

    /**
     * @brief Set callback for file/directory removal events
     * @param callback Function to call when files are removed
     */
    void on_remove(event_callback callback);

    /**
     * @brief Set callback for file/directory rename events
     * @param callback Function to call when files are renamed
     */
    void on_rename(event_callback callback);

    /**
     * @brief Set callback for error events
     * @param callback Function to call on errors
     */
    void on_error(error_callback callback);

    /**
     * @brief Set a filter function for events
     * @param filter Function that returns true for paths to include
     */
    void set_filter(filter_callback filter);

    /**
     * @brief Start watching for events
     * @return Result indicating success or error
     */
    [[nodiscard]] void_result start();

    /**
     * @brief Stop watching for events
     */
    void stop();

    /**
     * @brief Check if the watcher is currently running
     * @return True if running, false otherwise
     */
    [[nodiscard]] bool is_running() const noexcept;

    /**
     * @brief Get the list of currently watched paths
     * @return Vector of watched paths
     */
    [[nodiscard]] std::vector<path> watched_paths() const;

    /**
     * @brief Get the current watch options
     * @return Reference to the watch options
     */
    [[nodiscard]] const watch_options &options() const noexcept;

  private:
    class impl;
    std::unique_ptr<impl> _impl;
};

// ============================================================================
// 9.5 Convenience Functions
// ============================================================================

/**
 * @brief Synchronously wait for a single file change event
 * @param p Path to watch
 * @param timeout Maximum time to wait (0 = infinite)
 * @return The watch event or an error
 */
[[nodiscard]] FRAPPE_API result<watch_event>
wait_for_change(const path &p, std::chrono::milliseconds timeout = std::chrono::milliseconds{0}) noexcept;

/**
 * @brief Check if a path supports native file watching
 * @param p Path to check
 * @return True if native watching is supported, false otherwise
 */
[[nodiscard]] FRAPPE_API bool is_watch_supported(const path &p) noexcept;

} // namespace frappe

#endif // FRAPPE_WATCHER_HPP
