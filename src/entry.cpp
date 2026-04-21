#include "frappe/entry.hpp"

#include "frappe/attributes.hpp"

#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace frappe {

namespace {
bool is_hidden_name(const path &p) noexcept {
    auto name = p.filename().string();
    if (name.empty()) {
        return false;
    }
#ifdef _WIN32
    // On Windows, check file attributes
    DWORD attrs = GetFileAttributesW(p.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES) {
        return (attrs & FILE_ATTRIBUTE_HIDDEN) != 0;
    }
    return name[0] == '.';
#else
    return name[0] == '.';
#endif
}
} // namespace

bool is_hidden_file(const path &p) noexcept {
    return is_hidden_name(p);
}

char file_type_indicator(file_type ft) noexcept {
    switch (ft) {
    case file_type::directory:
        return '/';
    case file_type::symlink:
        return '@';
    case file_type::fifo:
        return '|';
    case file_type::socket:
        return '=';
    case file_type::block:
        return '#';
    case file_type::character:
        return '%';
    default:
        return ' ';
    }
}

char file_type_indicator(const file_entry &e) noexcept {
    if (e.is_symlink) {
        return '@';
    }
    if (e.type == file_type::regular) {
        // Check if executable
        if ((e.permissions & perms::owner_exec) != perms::none || (e.permissions & perms::group_exec) != perms::none ||
            (e.permissions & perms::others_exec) != perms::none) {
            return '*';
        }
        return ' ';
    }
    return file_type_indicator(e.type);
}

result<file_entry> get_file_entry(const path &p) noexcept {
    file_entry entry;
    std::error_code ec;

    // Basic path info
    entry.file_path = p;
    entry.name = p.filename().string();
    entry.stem = p.stem().string();
    entry.extension = p.extension().string();

    // Get status
    auto st = std::filesystem::symlink_status(p, ec);
    if (ec) {
        return std::unexpected(ec);
    }

    entry.type = st.type();
    entry.permissions = st.permissions();
    entry.is_symlink = (st.type() == file_type::symlink);
    entry.is_hidden = is_hidden_name(p);

    // Size (only for regular files)
    if (st.type() == file_type::regular) {
        entry.size = std::filesystem::file_size(p, ec);
        if (ec) {
            entry.size = 0;
        }
    }

    // Hard link count
    entry.hard_link_count = std::filesystem::hard_link_count(p, ec);
    if (ec) {
        entry.hard_link_count = 1;
    }

    // Timestamps
    entry.mtime = std::filesystem::last_write_time(p, ec);

    auto atime_result = last_access_time(p);
    if (atime_result) {
        entry.atime = *atime_result;
    }

    auto ctime_result = creation_time(p);
    if (ctime_result) {
        entry.ctime = *ctime_result;
        entry.birth_time = *ctime_result;
    }

    // Ownership
    auto owner_result = get_owner(p);
    if (owner_result) {
        entry.owner = owner_result->user_name;
        entry.group = owner_result->group_name;
        entry.uid = owner_result->uid;
        entry.gid = owner_result->gid;
    }

    // Symlink target
    if (entry.is_symlink) {
        auto target = std::filesystem::read_symlink(p, ec);
        if (!ec) {
            entry.symlink_target = target;
            entry.is_broken_link = !std::filesystem::exists(p, ec);
        }
    }

    // Inode
    auto id_result = get_file_id(p);
    if (id_result) {
        entry.inode = id_result->inode;
    }

    // Extended attributes check
    auto xattr_result = list_xattr(p);
    if (xattr_result) {
        entry.has_xattr = !xattr_result->empty();
    }

    // ACL check
    auto acl_result = has_acl(p);
    if (acl_result) {
        entry.has_acl = *acl_result;
    }

    // MIME type (lazy - only detect if needed)
    // entry.mime_type = detect_mime_type(p).value_or("");

    // Filesystem type
    auto fs_result = get_filesystem_type(p);
    if (fs_result) {
        entry.fs_type = *fs_result;
    }

    return entry;
}

file_entry get_file_entry(const std::filesystem::directory_entry &dir_entry) noexcept {
    file_entry entry;
    std::error_code ec;

    const auto &p = dir_entry.path();

    // Basic path info
    entry.file_path = p;
    entry.name = p.filename().string();
    entry.stem = p.stem().string();
    entry.extension = p.extension().string();

    // Use cached status from directory_entry
    auto st = dir_entry.symlink_status(ec);
    if (!ec) {
        entry.type = st.type();
        entry.permissions = st.permissions();
        entry.is_symlink = (st.type() == file_type::symlink);
    }

    entry.is_hidden = is_hidden_name(p);

    // Size
    if (dir_entry.is_regular_file(ec)) {
        entry.size = dir_entry.file_size(ec);
        if (ec) {
            entry.size = 0;
        }
    }

    // Hard link count
    entry.hard_link_count = dir_entry.hard_link_count(ec);
    if (ec) {
        entry.hard_link_count = 1;
    }

    // Timestamps
    entry.mtime = dir_entry.last_write_time(ec);

    auto atime_result = last_access_time(p);
    if (atime_result) {
        entry.atime = *atime_result;
    }

    auto ctime_result = creation_time(p);
    if (ctime_result) {
        entry.ctime = *ctime_result;
        entry.birth_time = *ctime_result;
    }

    // Ownership
    auto owner_result = get_owner(p);
    if (owner_result) {
        entry.owner = owner_result->user_name;
        entry.group = owner_result->group_name;
        entry.uid = owner_result->uid;
        entry.gid = owner_result->gid;
    }

    // Symlink target
    if (entry.is_symlink) {
        auto target = std::filesystem::read_symlink(p, ec);
        if (!ec) {
            entry.symlink_target = target;
            entry.is_broken_link = !std::filesystem::exists(p, ec);
        }
    }

    // Inode
    auto id_result = get_file_id(p);
    if (id_result) {
        entry.inode = id_result->inode;
    }

    return entry;
}

std::vector<file_entry> list_entries(const path &dir) {
    return list_entries(dir, list_options{});
}

std::vector<file_entry> list_entries(const path &dir, const list_options &opts) {
    std::vector<file_entry> result;
    std::error_code ec;

    for (const auto &entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) {
            continue;
        }

        auto file_entry = get_file_entry(entry);

        // Filter hidden files
        if (!opts.include_hidden && file_entry.is_hidden) {
            continue;
        }

        result.push_back(std::move(file_entry));
    }

    return result;
}

std::vector<file_entry> list_entries_recursive(const path &dir) {
    return list_entries_recursive(dir, list_options{});
}

std::vector<file_entry> list_entries_recursive(const path &dir, const list_options &opts) {
    std::vector<file_entry> result;
    std::error_code ec;

    auto dir_opts = std::filesystem::directory_options::none;
    if (opts.skip_permission_denied) {
        dir_opts |= std::filesystem::directory_options::skip_permission_denied;
    }
    if (opts.follow_symlinks) {
        dir_opts |= std::filesystem::directory_options::follow_directory_symlink;
    }

    for (const auto &entry : std::filesystem::recursive_directory_iterator(dir, dir_opts, ec)) {
        if (ec) {
            continue;
        }

        auto file_entry = get_file_entry(entry);

        // Filter hidden files
        if (!opts.include_hidden && file_entry.is_hidden) {
            continue;
        }

        result.push_back(std::move(file_entry));
    }

    return result;
}

#ifdef FRAPPE_HAS_GENERATOR
std::generator<file_entry> enumerate_entries(const path &dir) {
    return enumerate_entries(dir, list_options{});
}

std::generator<file_entry> enumerate_entries(const path &dir, const list_options &opts) {
    std::error_code ec;

    for (const auto &entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) {
            continue;
        }

        auto file_entry = get_file_entry(entry);

        if (!opts.include_hidden && file_entry.is_hidden) {
            continue;
        }

        co_yield std::move(file_entry);
    }
}

std::generator<file_entry> enumerate_entries_recursive(const path &dir) {
    return enumerate_entries_recursive(dir, list_options{});
}

std::generator<file_entry> enumerate_entries_recursive(const path &dir, const list_options &opts) {
    std::error_code ec;

    auto dir_opts = std::filesystem::directory_options::none;
    if (opts.skip_permission_denied) {
        dir_opts |= std::filesystem::directory_options::skip_permission_denied;
    }
    if (opts.follow_symlinks) {
        dir_opts |= std::filesystem::directory_options::follow_directory_symlink;
    }

    for (const auto &entry : std::filesystem::recursive_directory_iterator(dir, dir_opts, ec)) {
        if (ec) {
            continue;
        }

        auto file_entry = get_file_entry(entry);

        if (!opts.include_hidden && file_entry.is_hidden) {
            continue;
        }

        co_yield std::move(file_entry);
    }
}
#endif

} // namespace frappe
