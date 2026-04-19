#include "frappe/status.hpp"
#include <unordered_set>

namespace frappe {

namespace detail {
    result<file_time_type> creation_time_impl(const path& p) noexcept;
    result<file_time_type> last_access_time_impl(const path& p) noexcept;
    result<file_id> get_file_id_impl(const path& p) noexcept;
    result<std::uintmax_t> max_hard_links_impl(const path& p) noexcept;
}

// File type queries
result<file_status> status(const path& p) noexcept {
    std::error_code ec;
    auto st = std::filesystem::status(p, ec);
    if (ec) return std::unexpected(ec);
    return st;
}

result<file_status> symlink_status(const path& p) noexcept {
    std::error_code ec;
    auto st = std::filesystem::symlink_status(p, ec);
    if (ec) return std::unexpected(ec);
    return st;
}

result<bool> exists(const path& p) noexcept {
    std::error_code ec;
    bool r = std::filesystem::exists(p, ec);
    if (ec) return std::unexpected(ec);
    return r;
}

result<bool> is_regular_file(const path& p) noexcept {
    std::error_code ec;
    bool r = std::filesystem::is_regular_file(p, ec);
    if (ec) return std::unexpected(ec);
    return r;
}

result<bool> is_directory(const path& p) noexcept {
    std::error_code ec;
    bool r = std::filesystem::is_directory(p, ec);
    if (ec) return std::unexpected(ec);
    return r;
}

result<bool> is_symlink(const path& p) noexcept {
    std::error_code ec;
    bool r = std::filesystem::is_symlink(p, ec);
    if (ec) return std::unexpected(ec);
    return r;
}

result<bool> is_block_file(const path& p) noexcept {
    std::error_code ec;
    bool r = std::filesystem::is_block_file(p, ec);
    if (ec) return std::unexpected(ec);
    return r;
}

result<bool> is_character_file(const path& p) noexcept {
    std::error_code ec;
    bool r = std::filesystem::is_character_file(p, ec);
    if (ec) return std::unexpected(ec);
    return r;
}

result<bool> is_fifo(const path& p) noexcept {
    std::error_code ec;
    bool r = std::filesystem::is_fifo(p, ec);
    if (ec) return std::unexpected(ec);
    return r;
}

result<bool> is_socket(const path& p) noexcept {
    std::error_code ec;
    bool r = std::filesystem::is_socket(p, ec);
    if (ec) return std::unexpected(ec);
    return r;
}

result<bool> is_other(const path& p) noexcept {
    std::error_code ec;
    bool r = std::filesystem::is_other(p, ec);
    if (ec) return std::unexpected(ec);
    return r;
}

result<bool> is_empty(const path& p) noexcept {
    std::error_code ec;
    bool r = std::filesystem::is_empty(p, ec);
    if (ec) return std::unexpected(ec);
    return r;
}

// Basic attributes
result<std::uintmax_t> file_size(const path& p) noexcept {
    std::error_code ec;
    auto sz = std::filesystem::file_size(p, ec);
    if (ec) return std::unexpected(ec);
    return sz;
}

result<bool> equivalent(const path& p1, const path& p2) noexcept {
    std::error_code ec;
    bool r = std::filesystem::equivalent(p1, p2, ec);
    if (ec) return std::unexpected(ec);
    return r;
}

// Timestamps
result<file_time_type> last_write_time(const path& p) noexcept {
    std::error_code ec;
    auto t = std::filesystem::last_write_time(p, ec);
    if (ec) return std::unexpected(ec);
    return t;
}

void_result set_last_write_time(const path& p, file_time_type new_time) noexcept {
    std::error_code ec;
    std::filesystem::last_write_time(p, new_time, ec);
    if (ec) return std::unexpected(ec);
    return {};
}

result<file_time_type> creation_time(const path& p) noexcept {
    return detail::creation_time_impl(p);
}

result<file_time_type> last_access_time(const path& p) noexcept {
    return detail::last_access_time_impl(p);
}

// Symlink support
result<path> read_symlink(const path& p) noexcept {
    std::error_code ec;
    auto target = std::filesystem::read_symlink(p, ec);
    if (ec) return std::unexpected(ec);
    return target;
}

bool is_symlink_supported() noexcept {
    return true;
}

result<bool> is_symlink_supported(const path& p) noexcept {
    (void)p;
    return true;
}

result<bool> symlink_target_exists(const path& p) noexcept {
    std::error_code ec;
    auto st = std::filesystem::symlink_status(p, ec);
    if (ec) return std::unexpected(ec);
    if (st.type() != file_type::symlink) return false;
    return std::filesystem::exists(p, ec);
}

result<file_type> symlink_target_type(const path& p) noexcept {
    std::error_code ec;
    auto st = std::filesystem::symlink_status(p, ec);
    if (ec) return std::unexpected(ec);
    if (st.type() != file_type::symlink) return file_type::none;
    auto target_st = std::filesystem::status(p, ec);
    if (ec) return std::unexpected(ec);
    return target_st.type();
}

result<path> resolve_symlink(const path& p) noexcept {
    std::error_code ec;
    auto resolved = std::filesystem::canonical(p, ec);
    if (ec) return std::unexpected(ec);
    return resolved;
}

result<std::vector<path>> resolve_symlink_chain(const path& p) noexcept {
    std::vector<path> chain;
    std::error_code ec;
    
    path current = p;
    std::unordered_set<std::string> visited;
    
    while (true) {
        auto st = std::filesystem::symlink_status(current, ec);
        if (ec) return std::unexpected(ec);
        
        chain.push_back(current);
        
        if (st.type() != file_type::symlink) break;
        
        std::string current_str = current.string();
        if (visited.contains(current_str)) {
            return std::unexpected(make_error(std::errc::too_many_symbolic_link_levels));
        }
        visited.insert(current_str);
        
        auto target = std::filesystem::read_symlink(current, ec);
        if (ec) return std::unexpected(ec);
        
        if (target.is_relative()) {
            current = current.parent_path() / target;
        } else {
            current = target;
        }
    }
    
    return chain;
}

result<bool> is_broken_symlink(const path& p) noexcept {
    std::error_code ec;
    auto st = std::filesystem::symlink_status(p, ec);
    if (ec) return std::unexpected(ec);
    if (st.type() != file_type::symlink) return false;
    return !std::filesystem::exists(p, ec);
}

result<bool> is_circular_symlink(const path& p) noexcept {
    std::error_code ec;
    auto st = std::filesystem::symlink_status(p, ec);
    if (ec) return std::unexpected(ec);
    if (st.type() != file_type::symlink) return false;
    
    std::unordered_set<std::string> visited;
    path current = p;
    
    while (true) {
        st = std::filesystem::symlink_status(current, ec);
        if (ec) return std::unexpected(ec);
        
        if (st.type() != file_type::symlink) return false;
        
        std::string current_str = std::filesystem::absolute(current, ec).string();
        if (ec) return std::unexpected(ec);
        
        if (visited.contains(current_str)) return true;
        visited.insert(current_str);
        
        auto target = std::filesystem::read_symlink(current, ec);
        if (ec) return std::unexpected(ec);
        
        if (target.is_relative()) {
            current = current.parent_path() / target;
        } else {
            current = target;
        }
    }
}

// Symlink creation (compatible with std::filesystem)
void_result create_symlink(const path& target, const path& link) noexcept {
    std::error_code ec;
    std::filesystem::create_symlink(target, link, ec);
    if (ec) return std::unexpected(ec);
    return {};
}

void_result create_directory_symlink(const path& target, const path& link) noexcept {
    std::error_code ec;
    std::filesystem::create_directory_symlink(target, link, ec);
    if (ec) return std::unexpected(ec);
    return {};
}

// Hard link support
result<std::uintmax_t> hard_link_count(const path& p) noexcept {
    std::error_code ec;
    auto count = std::filesystem::hard_link_count(p, ec);
    if (ec) return std::unexpected(ec);
    return count;
}

result<bool> is_hard_link(const path& p) noexcept {
    std::error_code ec;
    auto count = std::filesystem::hard_link_count(p, ec);
    if (ec) return std::unexpected(ec);
    return count > 1;
}

bool is_hard_link_supported() noexcept {
    return true;
}

result<bool> is_hard_link_supported(const path& p) noexcept {
    (void)p;
    return true;
}

result<bool> are_hard_links(const path& p1, const path& p2) noexcept {
    std::error_code ec;
    bool r = std::filesystem::equivalent(p1, p2, ec);
    if (ec) return std::unexpected(ec);
    return r;
}

result<std::uintmax_t> max_hard_links(const path& p) noexcept {
    return detail::max_hard_links_impl(p);
}

result<std::vector<path>> find_hard_links(const path& p, const path& search_dir) noexcept {
    std::vector<path> results;
    
    auto target_id = get_file_id(p);
    if (!target_id) return std::unexpected(target_id.error());
    
    std::error_code ec;
    for (auto& entry : std::filesystem::recursive_directory_iterator(search_dir, 
            std::filesystem::directory_options::skip_permission_denied, ec)) {
        if (ec) continue;
        
        if (!entry.is_regular_file(ec)) continue;
        if (ec) continue;
        
        auto entry_id = get_file_id(entry.path());
        if (entry_id && *entry_id == *target_id) {
            results.push_back(entry.path());
        }
    }
    
    return results;
}

// Hard link creation (compatible with std::filesystem)
void_result create_hard_link(const path& target, const path& link) noexcept {
    std::error_code ec;
    std::filesystem::create_hard_link(target, link, ec);
    if (ec) return std::unexpected(ec);
    return {};
}

// File ID
result<file_id> get_file_id(const path& p) noexcept {
    return detail::get_file_id_impl(p);
}

// Link info
result<link_info> get_link_info(const path& p) noexcept {
    link_info info;
    std::error_code ec;
    
    auto st = std::filesystem::symlink_status(p, ec);
    if (ec) return std::unexpected(ec);
    
    if (st.type() == file_type::symlink) {
        info.type = link_type::symlink;
        info.target = std::filesystem::read_symlink(p, ec);
        info.is_broken = !std::filesystem::exists(p, ec);
        auto circular = is_circular_symlink(p);
        info.is_circular = circular.value_or(false);
    } else {
        info.link_count = std::filesystem::hard_link_count(p, ec);
        if (!ec && info.link_count > 1) {
            info.type = link_type::hardlink;
        }
    }
    
    auto id_result = get_file_id(p);
    if (id_result) {
        info.id = *id_result;
    }
    
    return info;
}

// File type name
std::string_view file_type_name(file_type ft) noexcept {
    switch (ft) {
        case file_type::none: return "none";
        case file_type::not_found: return "not_found";
        case file_type::regular: return "regular";
        case file_type::directory: return "directory";
        case file_type::symlink: return "symlink";
        case file_type::block: return "block";
        case file_type::character: return "character";
        case file_type::fifo: return "fifo";
        case file_type::socket: return "socket";
        case file_type::unknown: return "unknown";
        default: return "unknown";
    }
}

} // namespace frappe
