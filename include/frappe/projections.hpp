#ifndef FRAPPE_PROJECTIONS_HPP
#define FRAPPE_PROJECTIONS_HPP

#include "frappe/entry.hpp"
#include <functional>

namespace frappe {
namespace proj {

// Basic attribute projections - used for sorting, filtering, and selection

inline constexpr auto file_path = [](const file_entry& e) -> const path& { return e.file_path; };
inline constexpr auto name = [](const file_entry& e) -> const std::string& { return e.name; };
inline constexpr auto stem = [](const file_entry& e) -> const std::string& { return e.stem; };
inline constexpr auto extension = [](const file_entry& e) -> const std::string& { return e.extension; };

inline constexpr auto type = [](const file_entry& e) { return e.type; };
inline constexpr auto is_symlink = [](const file_entry& e) { return e.is_symlink; };
inline constexpr auto is_hidden = [](const file_entry& e) { return e.is_hidden; };

inline constexpr auto size = [](const file_entry& e) { return e.size; };
inline constexpr auto hard_link_count = [](const file_entry& e) { return e.hard_link_count; };
inline constexpr auto inode = [](const file_entry& e) { return e.inode; };

inline constexpr auto mtime = [](const file_entry& e) { return e.mtime; };
inline constexpr auto atime = [](const file_entry& e) { return e.atime; };
inline constexpr auto ctime = [](const file_entry& e) { return e.ctime; };
inline constexpr auto birth_time = [](const file_entry& e) { return e.birth_time; };

inline constexpr auto permissions = [](const file_entry& e) { return e.permissions; };
inline constexpr auto owner = [](const file_entry& e) -> const std::string& { return e.owner; };
inline constexpr auto group = [](const file_entry& e) -> const std::string& { return e.group; };
inline constexpr auto uid = [](const file_entry& e) { return e.uid; };
inline constexpr auto gid = [](const file_entry& e) { return e.gid; };

inline constexpr auto symlink_target = [](const file_entry& e) -> const path& { return e.symlink_target; };
inline constexpr auto is_broken_link = [](const file_entry& e) { return e.is_broken_link; };

inline constexpr auto has_acl = [](const file_entry& e) { return e.has_acl; };
inline constexpr auto has_xattr = [](const file_entry& e) { return e.has_xattr; };
inline constexpr auto mime_type = [](const file_entry& e) -> const std::string& { return e.mime_type; };

inline constexpr auto fs_type = [](const file_entry& e) { return e.fs_type; };

// Derived projections
inline constexpr auto parent_path = [](const file_entry& e) { return e.file_path.parent_path(); };
inline constexpr auto depth = [](const file_entry& e) { 
    return static_cast<int>(std::distance(e.file_path.begin(), e.file_path.end())); 
};

// Boolean projections for filtering
inline constexpr auto is_file = [](const file_entry& e) { return e.type == file_type::regular; };
inline constexpr auto is_directory = [](const file_entry& e) { return e.type == file_type::directory; };
inline constexpr auto is_regular = [](const file_entry& e) { return e.type == file_type::regular; };
inline constexpr auto is_block = [](const file_entry& e) { return e.type == file_type::block; };
inline constexpr auto is_character = [](const file_entry& e) { return e.type == file_type::character; };
inline constexpr auto is_fifo = [](const file_entry& e) { return e.type == file_type::fifo; };
inline constexpr auto is_socket = [](const file_entry& e) { return e.type == file_type::socket; };
inline constexpr auto is_empty = [](const file_entry& e) { return e.size == 0; };

// Permission check projections
inline constexpr auto is_readable = [](const file_entry& e) { 
    return (e.permissions & perms::owner_read) != perms::none; 
};
inline constexpr auto is_writable = [](const file_entry& e) { 
    return (e.permissions & perms::owner_write) != perms::none; 
};
inline constexpr auto is_executable = [](const file_entry& e) { 
    return (e.permissions & perms::owner_exec) != perms::none; 
};

} // namespace proj
} // namespace frappe

#endif // FRAPPE_PROJECTIONS_HPP
