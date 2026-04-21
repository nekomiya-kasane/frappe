#ifndef FRAPPE_ENTRY_HPP
#define FRAPPE_ENTRY_HPP

#include "frappe/error.hpp"
#include "frappe/exports.hpp"
#include "frappe/filesystem.hpp"
#include "frappe/permissions.hpp"
#include "frappe/status.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>
#if __has_include(<generator>)
#include <generator>
#define FRAPPE_HAS_GENERATOR 1
#endif

namespace frappe {

    using path = std::filesystem::path;
    using file_type = std::filesystem::file_type;
    using file_time_type = std::filesystem::file_time_type;
    using perms = std::filesystem::perms;

    // ============================================================================
    // Core file entry structure - aggregates all file attributes
    // ============================================================================

    struct file_entry {
        // Basic info
        path file_path;
        std::string name;      // filename
        std::string stem;      // filename without extension
        std::string extension; // extension with dot

        // Type info
        file_type type = file_type::unknown;
        bool is_symlink = false;
        bool is_hidden = false;

        // Size and links
        std::uintmax_t size = 0;
        std::uintmax_t hard_link_count = 1;
        std::uint64_t inode = 0;

        // Timestamps
        file_time_type mtime{};      // modification time
        file_time_type atime{};      // access time
        file_time_type ctime{};      // status change time (POSIX) / creation time (Windows)
        file_time_type birth_time{}; // creation time

        // Permissions and ownership
        perms permissions{};
        std::string owner;
        std::string group;
        std::uint32_t uid = 0;
        std::uint32_t gid = 0;

        // Symlink info
        path symlink_target;
        bool is_broken_link = false;

        // Extended attributes
        bool has_acl = false;
        bool has_xattr = false;
        std::string mime_type;

        // Filesystem info
        filesystem_type fs_type = filesystem_type::unknown;

        // Comparison operators for sorting
        auto operator<=>(const file_entry &other) const noexcept { return file_path <=> other.file_path; }
        bool operator==(const file_entry &other) const noexcept { return file_path == other.file_path; }
    };

    // ============================================================================
    // Factory functions
    // ============================================================================

    [[nodiscard]] FRAPPE_API result<file_entry> get_file_entry(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API file_entry get_file_entry(const std::filesystem::directory_entry &entry) noexcept;

    // ============================================================================
    // Directory listing - returns file_entry generator
    // ============================================================================

    struct list_options {
        bool include_hidden = false;
        bool follow_symlinks = false;
        bool skip_permission_denied = true;
        int max_depth = -1; // -1 = unlimited
    };

    [[nodiscard]] FRAPPE_API std::vector<file_entry> list_entries(const path &dir);
    [[nodiscard]] FRAPPE_API std::vector<file_entry> list_entries(const path &dir, const list_options &opts);
    [[nodiscard]] FRAPPE_API std::vector<file_entry> list_entries_recursive(const path &dir);
    [[nodiscard]] FRAPPE_API std::vector<file_entry> list_entries_recursive(const path &dir, const list_options &opts);

#ifdef FRAPPE_HAS_GENERATOR
    // Generator-based lazy iteration (C++23)
    [[nodiscard]] FRAPPE_API std::generator<file_entry> enumerate_entries(const path &dir);
    [[nodiscard]] FRAPPE_API std::generator<file_entry> enumerate_entries(const path &dir, const list_options &opts);
    [[nodiscard]] FRAPPE_API std::generator<file_entry> enumerate_entries_recursive(const path &dir);
    [[nodiscard]] FRAPPE_API std::generator<file_entry> enumerate_entries_recursive(const path &dir,
                                                                                    const list_options &opts);
#endif

    // ============================================================================
    // Utility functions
    // ============================================================================

    [[nodiscard]] FRAPPE_API bool is_hidden_file(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API char file_type_indicator(file_type ft) noexcept;
    [[nodiscard]] FRAPPE_API char file_type_indicator(const file_entry &e) noexcept;

} // namespace frappe

#endif // FRAPPE_ENTRY_HPP
