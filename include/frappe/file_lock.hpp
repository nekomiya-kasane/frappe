#ifndef FRAPPE_FILE_LOCK_HPP
#define FRAPPE_FILE_LOCK_HPP

#include "frappe/error.hpp"
#include "frappe/exports.hpp"

#include <chrono>
#include <filesystem>
#include <memory>

namespace frappe {

using path = std::filesystem::path;

// ============================================================================
// 9.1 Lock Types
// ============================================================================

/**
 * @brief Type of file lock
 */
enum class lock_type {
    shared,   ///< Shared lock (read lock)
    exclusive ///< Exclusive lock (write lock)
};

/**
 * @brief Lock acquisition mode
 */
enum class lock_mode {
    blocking,     ///< Blocking wait until lock is acquired
    non_blocking, ///< Non-blocking (return immediately)
    try_for       ///< Wait with timeout
};

/**
 * @brief Options for file locking operations
 */
struct lock_options {
    lock_type type = lock_type::exclusive; ///< Type of lock to acquire
    lock_mode mode = lock_mode::blocking;  ///< Lock acquisition mode
    std::chrono::milliseconds timeout{0};  ///< Timeout for try_for mode
    std::size_t offset = 0;                ///< Lock start offset
    std::size_t length = 0;                ///< Lock length (0 = entire file)
};

// ============================================================================
// 9.2 File Lock Class
// ============================================================================

/**
 * @brief File lock class for cross-platform file locking
 *
 * This class provides file locking functionality with support for
 * shared and exclusive locks, as well as blocking and non-blocking modes.
 */
class FRAPPE_API file_lock {
  public:
    /**
     * @brief Construct a file lock for the specified path
     * @param p Path to the file to lock
     * @param options Lock options
     */
    explicit file_lock(const path &p, lock_options options = {});

    /**
     * @brief Destructor - releases the lock if held
     */
    ~file_lock();

    // Non-copyable
    file_lock(const file_lock &) = delete;
    file_lock &operator=(const file_lock &) = delete;

    // Movable
    file_lock(file_lock &&other) noexcept;
    file_lock &operator=(file_lock &&other) noexcept;

    /**
     * @brief Acquire the lock (blocking)
     * @return Result indicating success or error
     */
    [[nodiscard]] void_result lock();

    /**
     * @brief Try to acquire the lock without blocking
     * @return Result indicating success or error
     */
    [[nodiscard]] void_result try_lock();

    /**
     * @brief Try to acquire the lock with timeout
     * @param timeout Maximum time to wait for the lock
     * @return Result indicating success or error
     */
    [[nodiscard]] void_result try_lock_for(std::chrono::milliseconds timeout);

    /**
     * @brief Release the lock
     */
    void unlock();

    /**
     * @brief Check if this object owns the lock
     * @return True if the lock is held
     */
    [[nodiscard]] bool owns_lock() const noexcept;

    /**
     * @brief Boolean conversion operator
     * @return True if the lock is held
     */
    explicit operator bool() const noexcept;

    /**
     * @brief Get the path of the locked file
     * @return Reference to the file path
     */
    [[nodiscard]] const path &file_path() const noexcept;

    /**
     * @brief Get the lock type
     * @return The type of lock (shared or exclusive)
     */
    [[nodiscard]] lock_type type() const noexcept;

  private:
    class impl;
    std::unique_ptr<impl> _impl;
};

// ============================================================================
// 9.3 RAII Lock Guard
// ============================================================================

/**
 * @brief RAII-style scoped file lock
 *
 * Automatically acquires the lock on construction and releases it on destruction.
 * This class is neither copyable nor movable.
 */
class FRAPPE_API scoped_file_lock {
  public:
    /**
     * @brief Construct and acquire a scoped file lock
     * @param p Path to the file to lock
     * @param type Type of lock to acquire
     */
    explicit scoped_file_lock(const path &p, lock_type type = lock_type::exclusive);

    /**
     * @brief Destructor - releases the lock
     */
    ~scoped_file_lock();

    // Non-copyable, non-movable
    scoped_file_lock(const scoped_file_lock &) = delete;
    scoped_file_lock &operator=(const scoped_file_lock &) = delete;
    scoped_file_lock(scoped_file_lock &&) = delete;
    scoped_file_lock &operator=(scoped_file_lock &&) = delete;

    /**
     * @brief Check if the lock is held
     * @return True if the lock was successfully acquired
     */
    [[nodiscard]] bool owns_lock() const noexcept;

    /**
     * @brief Boolean conversion operator
     * @return True if the lock is held
     */
    explicit operator bool() const noexcept;

  private:
    file_lock _lock;
    bool _locked = false;
};

// ============================================================================
// 9.4 Convenience Functions
// ============================================================================

/**
 * @brief Lock a file with the specified options
 * @param p Path to the file to lock
 * @param options Lock options
 * @return Result containing the file lock or an error
 */
[[nodiscard]] FRAPPE_API result<file_lock> lock_file(const path &p, lock_options options = {}) noexcept;

/**
 * @brief Try to lock a file without blocking
 * @param p Path to the file to lock
 * @param type Type of lock to acquire
 * @return Result containing true if lock was acquired, false otherwise, or an error
 */
[[nodiscard]] FRAPPE_API result<bool> try_lock_file(const path &p, lock_type type = lock_type::exclusive) noexcept;

/**
 * @brief Check if a file is currently locked
 * @param p Path to the file to check
 * @return Result containing true if the file is locked, or an error
 */
[[nodiscard]] FRAPPE_API result<bool> is_file_locked(const path &p) noexcept;

/**
 * @brief Get the human-readable name of a lock type
 * @param type The lock type
 * @return String view of the lock type name
 */
[[nodiscard]] FRAPPE_API std::string_view lock_type_name(lock_type type) noexcept;

} // namespace frappe

#endif // FRAPPE_FILE_LOCK_HPP
