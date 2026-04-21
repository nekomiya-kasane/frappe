#ifndef FRAPPE_STATUS_HPP
#define FRAPPE_STATUS_HPP

#include "frappe/error.hpp"
#include "frappe/exports.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace frappe {

    using path = std::filesystem::path;
    using file_type = std::filesystem::file_type;
    using file_status = std::filesystem::file_status;
    using file_time_type = std::filesystem::file_time_type;

    // File type queries
    [[nodiscard]] FRAPPE_API result<file_status> status(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<file_status> symlink_status(const path &p) noexcept;

    [[nodiscard]] FRAPPE_API result<bool> exists(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_regular_file(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_directory(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_symlink(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_block_file(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_character_file(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_fifo(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_socket(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_other(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_empty(const path &p) noexcept;

    // Basic attributes
    [[nodiscard]] FRAPPE_API result<std::uintmax_t> file_size(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> equivalent(const path &p1, const path &p2) noexcept;

    // Timestamps
    [[nodiscard]] FRAPPE_API result<file_time_type> last_write_time(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API void_result set_last_write_time(const path &p, file_time_type new_time) noexcept;

    [[nodiscard]] FRAPPE_API result<file_time_type> creation_time(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<file_time_type> last_access_time(const path &p) noexcept;

    // Symlink support - queries
    [[nodiscard]] FRAPPE_API result<path> read_symlink(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API bool is_symlink_supported() noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_symlink_supported(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> symlink_target_exists(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<file_type> symlink_target_type(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<path> resolve_symlink(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<std::vector<path>> resolve_symlink_chain(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_broken_symlink(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_circular_symlink(const path &p) noexcept;

    // Symlink support - creation (compatible with std::filesystem)
    [[nodiscard]] FRAPPE_API void_result create_symlink(const path &target, const path &link) noexcept;
    [[nodiscard]] FRAPPE_API void_result create_directory_symlink(const path &target, const path &link) noexcept;

    // Hard link support - queries
    [[nodiscard]] FRAPPE_API result<std::uintmax_t> hard_link_count(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_hard_link(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API bool is_hard_link_supported() noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_hard_link_supported(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> are_hard_links(const path &p1, const path &p2) noexcept;
    [[nodiscard]] FRAPPE_API result<std::uintmax_t> max_hard_links(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<std::vector<path>> find_hard_links(const path &p, const path &search_dir) noexcept;

    // Hard link support - creation (compatible with std::filesystem)
    [[nodiscard]] FRAPPE_API void_result create_hard_link(const path &target, const path &link) noexcept;

    // File ID (inode on POSIX, file index on Windows)
    struct file_id {
        std::uint64_t device = 0;
        std::uint64_t inode = 0;

        bool operator==(const file_id &other) const noexcept { return device == other.device && inode == other.inode; }
        bool operator!=(const file_id &other) const noexcept { return !(*this == other); }
    };

    [[nodiscard]] FRAPPE_API result<file_id> get_file_id(const path &p) noexcept;

    // Link info structure
    enum class link_type { none, symlink, hardlink };

    struct link_info {
        link_type type = link_type::none;
        path target;
        std::uintmax_t link_count = 0;
        file_id id{};
        bool is_broken = false;
        bool is_circular = false;
    };

    [[nodiscard]] FRAPPE_API result<link_info> get_link_info(const path &p) noexcept;

    // File type to string conversion
    [[nodiscard]] FRAPPE_API std::string_view file_type_name(file_type ft) noexcept;

} // namespace frappe

#endif // FRAPPE_STATUS_HPP
