#include "frappe/permissions.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace frappe {

namespace detail {
result<owner_info> get_owner_impl(const path &p) noexcept;
void_result set_owner_impl(const path &p, std::uint32_t uid, std::uint32_t gid) noexcept;
std::optional<std::uint32_t> user_id_from_name_impl(std::string_view name) noexcept;
std::optional<std::string> user_name_from_id_impl(std::uint32_t uid) noexcept;
std::optional<std::uint32_t> group_id_from_name_impl(std::string_view name) noexcept;
std::optional<std::string> group_name_from_id_impl(std::uint32_t gid) noexcept;
std::uint32_t current_user_id_impl() noexcept;
std::uint32_t current_group_id_impl() noexcept;
std::string current_user_name_impl() noexcept;
std::string current_group_name_impl() noexcept;
std::vector<std::uint32_t> current_user_groups_impl() noexcept;
perms get_umask_impl() noexcept;
void set_umask_impl(perms mask) noexcept;
result<bool> check_access_impl(const path &p, access_rights rights) noexcept;
result<access_rights> get_effective_rights_impl(const path &p) noexcept;
} // namespace detail

// POSIX permissions
result<perms> get_permissions(const path &p) noexcept {
    std::error_code ec;
    auto st = std::filesystem::status(p, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return st.permissions();
}

void_result set_permissions(const path &p, perms prms, perm_options opts) noexcept {
    std::error_code ec;
    std::filesystem::permissions(p, prms, opts, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return {};
}

void_result add_permissions(const path &p, perms prms) noexcept {
    std::error_code ec;
    std::filesystem::permissions(p, prms, perm_options::add, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return {};
}

void_result remove_permissions(const path &p, perms prms) noexcept {
    std::error_code ec;
    std::filesystem::permissions(p, prms, perm_options::remove, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return {};
}

result<bool> has_permission(const path &p, perms prm) noexcept {
    auto current = get_permissions(p);
    if (!current) {
        return std::unexpected(current.error());
    }
    return (*current & prm) != perms::none;
}

result<bool> is_readable(const path &p) noexcept {
    return has_permission(p, perms::owner_read | perms::group_read | perms::others_read);
}

result<bool> is_writable(const path &p) noexcept {
    return has_permission(p, perms::owner_write | perms::group_write | perms::others_write);
}

result<bool> is_executable(const path &p) noexcept {
    return has_permission(p, perms::owner_exec | perms::group_exec | perms::others_exec);
}

std::string permissions_to_string(perms prms) noexcept {
    std::string result(9, '-');

    if ((prms & perms::owner_read) != perms::none) {
        result[0] = 'r';
    }
    if ((prms & perms::owner_write) != perms::none) {
        result[1] = 'w';
    }
    if ((prms & perms::owner_exec) != perms::none) {
        result[2] = 'x';
    }
    if ((prms & perms::group_read) != perms::none) {
        result[3] = 'r';
    }
    if ((prms & perms::group_write) != perms::none) {
        result[4] = 'w';
    }
    if ((prms & perms::group_exec) != perms::none) {
        result[5] = 'x';
    }
    if ((prms & perms::others_read) != perms::none) {
        result[6] = 'r';
    }
    if ((prms & perms::others_write) != perms::none) {
        result[7] = 'w';
    }
    if ((prms & perms::others_exec) != perms::none) {
        result[8] = 'x';
    }

    if ((prms & perms::set_uid) != perms::none) {
        result[2] = (result[2] == 'x') ? 's' : 'S';
    }
    if ((prms & perms::set_gid) != perms::none) {
        result[5] = (result[5] == 'x') ? 's' : 'S';
    }
    if ((prms & perms::sticky_bit) != perms::none) {
        result[8] = (result[8] == 'x') ? 't' : 'T';
    }

    return result;
}

std::string permissions_to_octal(perms prms) noexcept {
    unsigned mode = static_cast<unsigned>(prms) & 07777;
    std::ostringstream oss;
    oss << std::oct << std::setfill('0') << std::setw(4) << mode;
    return oss.str();
}

perms permissions_from_string(std::string_view str) noexcept {
    perms result = perms::none;
    if (str.size() < 9) {
        return result;
    }

    if (str[0] == 'r') {
        result |= perms::owner_read;
    }
    if (str[1] == 'w') {
        result |= perms::owner_write;
    }
    if (str[2] == 'x' || str[2] == 's') {
        result |= perms::owner_exec;
    }
    if (str[2] == 's' || str[2] == 'S') {
        result |= perms::set_uid;
    }
    if (str[3] == 'r') {
        result |= perms::group_read;
    }
    if (str[4] == 'w') {
        result |= perms::group_write;
    }
    if (str[5] == 'x' || str[5] == 's') {
        result |= perms::group_exec;
    }
    if (str[5] == 's' || str[5] == 'S') {
        result |= perms::set_gid;
    }
    if (str[6] == 'r') {
        result |= perms::others_read;
    }
    if (str[7] == 'w') {
        result |= perms::others_write;
    }
    if (str[8] == 'x' || str[8] == 't') {
        result |= perms::others_exec;
    }
    if (str[8] == 't' || str[8] == 'T') {
        result |= perms::sticky_bit;
    }

    return result;
}

perms permissions_from_octal(std::string_view octal) noexcept {
    unsigned mode = 0;
    for (char c : octal) {
        if (c >= '0' && c <= '7') {
            mode = mode * 8 + (c - '0');
        }
    }
    return static_cast<perms>(mode);
}

perms permissions_from_mode(unsigned mode) noexcept {
    return static_cast<perms>(mode & 07777);
}

perms get_umask() noexcept {
    return detail::get_umask_impl();
}

void set_umask(perms mask) noexcept {
    detail::set_umask_impl(mask);
}

perms apply_umask(perms prms) noexcept {
    auto mask = get_umask();
    return prms & ~mask;
}

result<bool> has_setuid(const path &p) noexcept {
    return has_permission(p, perms::set_uid);
}

result<bool> has_setgid(const path &p) noexcept {
    return has_permission(p, perms::set_gid);
}

result<bool> has_sticky_bit(const path &p) noexcept {
    return has_permission(p, perms::sticky_bit);
}

void_result set_setuid(const path &p, bool enable) noexcept {
    if (enable) {
        return add_permissions(p, perms::set_uid);
    } else {
        return remove_permissions(p, perms::set_uid);
    }
}

void_result set_setgid(const path &p, bool enable) noexcept {
    if (enable) {
        return add_permissions(p, perms::set_gid);
    } else {
        return remove_permissions(p, perms::set_gid);
    }
}

void_result set_sticky_bit(const path &p, bool enable) noexcept {
    if (enable) {
        return add_permissions(p, perms::sticky_bit);
    } else {
        return remove_permissions(p, perms::sticky_bit);
    }
}

// Ownership
result<owner_info> get_owner(const path &p) noexcept {
    return detail::get_owner_impl(p);
}

void_result set_owner(const path &p, std::uint32_t uid, std::uint32_t gid) noexcept {
    return detail::set_owner_impl(p, uid, gid);
}

void_result set_owner_user(const path &p, std::uint32_t uid) noexcept {
    auto info = get_owner(p);
    if (!info) {
        return std::unexpected(info.error());
    }
    return set_owner(p, uid, info->gid);
}

void_result set_owner_user(const path &p, std::string_view user_name) noexcept {
    auto uid = user_id_from_name(user_name);
    if (!uid) {
        return std::unexpected(make_error(std::errc::invalid_argument));
    }
    return set_owner_user(p, *uid);
}

void_result set_owner_group(const path &p, std::uint32_t gid) noexcept {
    auto info = get_owner(p);
    if (!info) {
        return std::unexpected(info.error());
    }
    return set_owner(p, info->uid, gid);
}

void_result set_owner_group(const path &p, std::string_view group_name) noexcept {
    auto gid = group_id_from_name(group_name);
    if (!gid) {
        return std::unexpected(make_error(std::errc::invalid_argument));
    }
    return set_owner_group(p, *gid);
}

// User/Group lookup
std::optional<std::uint32_t> user_id_from_name(std::string_view name) noexcept {
    return detail::user_id_from_name_impl(name);
}

std::optional<std::string> user_name_from_id(std::uint32_t uid) noexcept {
    return detail::user_name_from_id_impl(uid);
}

std::optional<std::uint32_t> group_id_from_name(std::string_view name) noexcept {
    return detail::group_id_from_name_impl(name);
}

std::optional<std::string> group_name_from_id(std::uint32_t gid) noexcept {
    return detail::group_name_from_id_impl(gid);
}

std::uint32_t current_user_id() noexcept {
    return detail::current_user_id_impl();
}

std::uint32_t current_group_id() noexcept {
    return detail::current_group_id_impl();
}

std::string current_user_name() noexcept {
    return detail::current_user_name_impl();
}

std::string current_group_name() noexcept {
    return detail::current_group_name_impl();
}

std::vector<std::uint32_t> current_user_groups() noexcept {
    return detail::current_user_groups_impl();
}

// Cross-platform access rights
result<bool> check_access(const path &p, access_rights rights) noexcept {
    return detail::check_access_impl(p, rights);
}

result<access_rights> get_effective_rights(const path &p) noexcept {
    return detail::get_effective_rights_impl(p);
}

// 4.4 User/Group existence and membership
bool user_exists(std::string_view name) noexcept {
    return user_id_from_name(name).has_value();
}

bool group_exists(std::string_view name) noexcept {
    return group_id_from_name(name).has_value();
}

std::vector<std::string> get_user_groups(std::string_view user_name) noexcept {
    std::vector<std::string> result;
    auto groups = current_user_groups();
    for (auto gid : groups) {
        auto name = group_name_from_id(gid);
        if (name) {
            result.push_back(*name);
        }
    }
    return result;
}

bool is_user_in_group(std::string_view user_name, std::string_view group_name) noexcept {
    auto groups = get_user_groups(user_name);
    for (const auto &g : groups) {
        if (g == group_name) {
            return true;
        }
    }
    return false;
}

// 4.7 Cross-platform access check for specific user
result<bool> check_access(const path &p, std::string_view user_name, access_rights rights) noexcept {
    // For now, only support current user check
    if (user_name == current_user_name()) {
        return check_access(p, rights);
    }
    return std::unexpected(make_error(std::errc::not_supported));
}

result<access_rights> get_effective_rights(const path &p, std::string_view user_name) noexcept {
    if (user_name == current_user_name()) {
        return get_effective_rights(p);
    }
    return std::unexpected(make_error(std::errc::not_supported));
}

// 4.5 ACL Support
std::string acl_to_string(const acl_info &acl) noexcept {
    std::ostringstream oss;
    for (const auto &entry : acl.entries) {
        switch (entry.tag) {
        case acl_tag::user_obj:
            oss << "user::";
            break;
        case acl_tag::user:
            oss << "user:" << entry.qualifier << ":";
            break;
        case acl_tag::group_obj:
            oss << "group::";
            break;
        case acl_tag::group:
            oss << "group:" << entry.qualifier << ":";
            break;
        case acl_tag::mask:
            oss << "mask::";
            break;
        case acl_tag::other:
            oss << "other::";
            break;
        }
        oss << (entry.read ? 'r' : '-');
        oss << (entry.write ? 'w' : '-');
        oss << (entry.execute ? 'x' : '-');
        oss << '\n';
    }
    return oss.str();
}

namespace detail {
result<acl_info> get_acl_impl(const path &p, acl_type type) noexcept;
void_result set_acl_impl(const path &p, const acl_info &acl, acl_type type) noexcept;
result<bool> has_acl_impl(const path &p) noexcept;
result<bool> is_acl_supported_impl(const path &p) noexcept;

result<win_attributes> get_win_attributes_impl(const path &p) noexcept;
void_result set_win_attributes_impl(const path &p, win_attributes attrs) noexcept;
} // namespace detail

result<acl_info> get_acl(const path &p) noexcept {
    return detail::get_acl_impl(p, acl_type::access);
}

result<acl_info> get_acl(const path &p, acl_type type) noexcept {
    return detail::get_acl_impl(p, type);
}

result<acl_info> get_default_acl(const path &p) noexcept {
    return detail::get_acl_impl(p, acl_type::default_acl);
}

result<bool> has_acl(const path &p) noexcept {
    return detail::has_acl_impl(p);
}

result<bool> has_default_acl(const path &p) noexcept {
    auto acl = get_default_acl(p);
    if (!acl) {
        return std::unexpected(acl.error());
    }
    return !acl->entries.empty();
}

result<bool> is_acl_supported(const path &p) noexcept {
    return detail::is_acl_supported_impl(p);
}

void_result set_acl(const path &p, const acl_info &acl) noexcept {
    return detail::set_acl_impl(p, acl, acl_type::access);
}

void_result set_default_acl(const path &p, const acl_info &acl) noexcept {
    return detail::set_acl_impl(p, acl, acl_type::default_acl);
}

void_result add_acl_entry(const path &p, const acl_entry &entry) noexcept {
    auto acl = get_acl(p);
    if (!acl) {
        return std::unexpected(acl.error());
    }
    acl->entries.push_back(entry);
    return set_acl(p, *acl);
}

void_result remove_acl_entry(const path &p, const acl_entry &entry) noexcept {
    auto acl = get_acl(p);
    if (!acl) {
        return std::unexpected(acl.error());
    }

    auto &entries = acl->entries;
    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
                       [&](const acl_entry &e) { return e.tag == entry.tag && e.qualifier == entry.qualifier; }),
        entries.end());
    return set_acl(p, *acl);
}

void_result remove_acl(const path &p) noexcept {
    acl_info empty_acl;
    return set_acl(p, empty_acl);
}

void_result remove_default_acl(const path &p) noexcept {
    acl_info empty_acl;
    return set_default_acl(p, empty_acl);
}

// 4.6 Windows Attributes
result<win_attributes> get_win_attributes(const path &p) noexcept {
    return detail::get_win_attributes_impl(p);
}

result<bool> is_readonly(const path &p) noexcept {
    auto attrs = get_win_attributes(p);
    if (!attrs) {
        return std::unexpected(attrs.error());
    }
    return has_attribute(*attrs, win_attributes::readonly);
}

result<bool> is_hidden(const path &p) noexcept {
    auto attrs = get_win_attributes(p);
    if (!attrs) {
        return std::unexpected(attrs.error());
    }
    return has_attribute(*attrs, win_attributes::hidden);
}

result<bool> is_system_file(const path &p) noexcept {
    auto attrs = get_win_attributes(p);
    if (!attrs) {
        return std::unexpected(attrs.error());
    }
    return has_attribute(*attrs, win_attributes::system);
}

result<bool> is_archive(const path &p) noexcept {
    auto attrs = get_win_attributes(p);
    if (!attrs) {
        return std::unexpected(attrs.error());
    }
    return has_attribute(*attrs, win_attributes::archive);
}

result<bool> is_compressed(const path &p) noexcept {
    auto attrs = get_win_attributes(p);
    if (!attrs) {
        return std::unexpected(attrs.error());
    }
    return has_attribute(*attrs, win_attributes::compressed);
}

result<bool> is_encrypted(const path &p) noexcept {
    auto attrs = get_win_attributes(p);
    if (!attrs) {
        return std::unexpected(attrs.error());
    }
    return has_attribute(*attrs, win_attributes::encrypted);
}

result<bool> is_sparse(const path &p) noexcept {
    auto attrs = get_win_attributes(p);
    if (!attrs) {
        return std::unexpected(attrs.error());
    }
    return has_attribute(*attrs, win_attributes::sparse);
}

result<bool> is_reparse_point(const path &p) noexcept {
    auto attrs = get_win_attributes(p);
    if (!attrs) {
        return std::unexpected(attrs.error());
    }
    return has_attribute(*attrs, win_attributes::reparse_point);
}

void_result set_win_attributes(const path &p, win_attributes attrs) noexcept {
    return detail::set_win_attributes_impl(p, attrs);
}

void_result add_win_attribute(const path &p, win_attributes attr) noexcept {
    auto current = get_win_attributes(p);
    if (!current) {
        return std::unexpected(current.error());
    }
    return set_win_attributes(p, *current | attr);
}

void_result remove_win_attribute(const path &p, win_attributes attr) noexcept {
    auto current = get_win_attributes(p);
    if (!current) {
        return std::unexpected(current.error());
    }
    auto new_attrs =
        static_cast<win_attributes>(static_cast<std::uint32_t>(*current) & ~static_cast<std::uint32_t>(attr));
    return set_win_attributes(p, new_attrs);
}

void_result set_readonly(const path &p, bool value) noexcept {
    if (value) {
        return add_win_attribute(p, win_attributes::readonly);
    }
    return remove_win_attribute(p, win_attributes::readonly);
}

void_result set_hidden(const path &p, bool value) noexcept {
    if (value) {
        return add_win_attribute(p, win_attributes::hidden);
    }
    return remove_win_attribute(p, win_attributes::hidden);
}

void_result set_system_file(const path &p, bool value) noexcept {
    if (value) {
        return add_win_attribute(p, win_attributes::system);
    }
    return remove_win_attribute(p, win_attributes::system);
}

void_result set_archive(const path &p, bool value) noexcept {
    if (value) {
        return add_win_attribute(p, win_attributes::archive);
    }
    return remove_win_attribute(p, win_attributes::archive);
}

// ============================================================================
// 4.4.1 Windows Security Identifier (SID) Support
// ============================================================================

std::string_view sid_type_name(sid_type type) noexcept {
    switch (type) {
    case sid_type::unknown:
        return "unknown";
    case sid_type::user:
        return "user";
    case sid_type::group:
        return "group";
    case sid_type::domain:
        return "domain";
    case sid_type::alias:
        return "alias";
    case sid_type::well_known_group:
        return "well_known_group";
    case sid_type::deleted_account:
        return "deleted_account";
    case sid_type::invalid:
        return "invalid";
    case sid_type::computer:
        return "computer";
    case sid_type::label:
        return "label";
    default:
        return "unknown";
    }
}

namespace detail {
result<security_identifier> get_sid_impl(std::string_view account_name) noexcept;
result<security_identifier> get_file_owner_sid_impl(const path &p) noexcept;
result<security_identifier> get_account_from_sid_impl(std::string_view sid_string) noexcept;
bool is_valid_sid_impl(std::string_view sid_string) noexcept;
std::string get_computer_name_impl() noexcept;
std::string get_domain_name_impl() noexcept;
bool is_domain_joined_impl() noexcept;
result<security_identifier> lookup_account_impl(std::string_view domain, std::string_view name) noexcept;
result<std::vector<std::string>> get_local_users_impl() noexcept;
result<std::vector<std::string>> get_local_groups_impl() noexcept;
} // namespace detail

result<security_identifier> get_sid(std::string_view account_name) noexcept {
    return detail::get_sid_impl(account_name);
}

result<security_identifier> get_file_owner_sid(const path &p) noexcept {
    return detail::get_file_owner_sid_impl(p);
}

result<security_identifier> get_account_from_sid(std::string_view sid_string) noexcept {
    return detail::get_account_from_sid_impl(sid_string);
}

bool is_valid_sid(std::string_view sid_string) noexcept {
    return detail::is_valid_sid_impl(sid_string);
}

std::string get_computer_name() noexcept {
    return detail::get_computer_name_impl();
}

std::string get_domain_name() noexcept {
    return detail::get_domain_name_impl();
}

bool is_domain_joined() noexcept {
    return detail::is_domain_joined_impl();
}

result<security_identifier> lookup_account(std::string_view domain, std::string_view name) noexcept {
    return detail::lookup_account_impl(domain, name);
}

result<std::vector<std::string>> get_local_users() noexcept {
    return detail::get_local_users_impl();
}

result<std::vector<std::string>> get_local_groups() noexcept {
    return detail::get_local_groups_impl();
}

} // namespace frappe
