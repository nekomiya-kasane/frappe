#ifndef FRAPPE_PERMISSIONS_HPP
#define FRAPPE_PERMISSIONS_HPP

#include "frappe/exports.hpp"
#include "frappe/error.hpp"
#include <filesystem>
#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <cstdint>

namespace frappe {

using path = std::filesystem::path;
using perms = std::filesystem::perms;
using perm_options = std::filesystem::perm_options;

// POSIX permissions
[[nodiscard]] FRAPPE_API result<perms> get_permissions(const path& p) noexcept;
[[nodiscard]] FRAPPE_API void_result set_permissions(const path& p, perms prms, perm_options opts = perm_options::replace) noexcept;
[[nodiscard]] FRAPPE_API void_result add_permissions(const path& p, perms prms) noexcept;
[[nodiscard]] FRAPPE_API void_result remove_permissions(const path& p, perms prms) noexcept;

// Permission queries
[[nodiscard]] FRAPPE_API result<bool> has_permission(const path& p, perms prm) noexcept;
[[nodiscard]] FRAPPE_API result<bool> is_readable(const path& p) noexcept;
[[nodiscard]] FRAPPE_API result<bool> is_writable(const path& p) noexcept;
[[nodiscard]] FRAPPE_API result<bool> is_executable(const path& p) noexcept;

// Permission string conversion (no errors possible)
[[nodiscard]] FRAPPE_API std::string permissions_to_string(perms prms) noexcept;
[[nodiscard]] FRAPPE_API std::string permissions_to_octal(perms prms) noexcept;
[[nodiscard]] FRAPPE_API perms permissions_from_string(std::string_view str) noexcept;
[[nodiscard]] FRAPPE_API perms permissions_from_octal(std::string_view octal) noexcept;
[[nodiscard]] FRAPPE_API perms permissions_from_mode(unsigned mode) noexcept;

// Umask
[[nodiscard]] FRAPPE_API perms get_umask() noexcept;
FRAPPE_API void set_umask(perms mask) noexcept;
[[nodiscard]] FRAPPE_API perms apply_umask(perms prms) noexcept;

// Special bits
[[nodiscard]] FRAPPE_API result<bool> has_setuid(const path& p) noexcept;
[[nodiscard]] FRAPPE_API result<bool> has_setgid(const path& p) noexcept;
[[nodiscard]] FRAPPE_API result<bool> has_sticky_bit(const path& p) noexcept;
[[nodiscard]] FRAPPE_API void_result set_setuid(const path& p, bool enable = true) noexcept;
[[nodiscard]] FRAPPE_API void_result set_setgid(const path& p, bool enable = true) noexcept;
[[nodiscard]] FRAPPE_API void_result set_sticky_bit(const path& p, bool enable = true) noexcept;

// Ownership
struct owner_info {
    std::uint32_t uid = 0;
    std::uint32_t gid = 0;
    std::string user_name;
    std::string group_name;
};

[[nodiscard]] FRAPPE_API result<owner_info> get_owner(const path& p) noexcept;
[[nodiscard]] FRAPPE_API void_result set_owner(const path& p, std::uint32_t uid, std::uint32_t gid) noexcept;
[[nodiscard]] FRAPPE_API void_result set_owner_user(const path& p, std::uint32_t uid) noexcept;
[[nodiscard]] FRAPPE_API void_result set_owner_user(const path& p, std::string_view user_name) noexcept;
[[nodiscard]] FRAPPE_API void_result set_owner_group(const path& p, std::uint32_t gid) noexcept;
[[nodiscard]] FRAPPE_API void_result set_owner_group(const path& p, std::string_view group_name) noexcept;

// User/Group lookup
[[nodiscard]] FRAPPE_API std::optional<std::uint32_t> user_id_from_name(std::string_view name) noexcept;
[[nodiscard]] FRAPPE_API std::optional<std::string> user_name_from_id(std::uint32_t uid) noexcept;
[[nodiscard]] FRAPPE_API std::optional<std::uint32_t> group_id_from_name(std::string_view name) noexcept;
[[nodiscard]] FRAPPE_API std::optional<std::string> group_name_from_id(std::uint32_t gid) noexcept;

// 4.4 User/Group existence and membership
[[nodiscard]] FRAPPE_API bool user_exists(std::string_view name) noexcept;
[[nodiscard]] FRAPPE_API bool group_exists(std::string_view name) noexcept;
[[nodiscard]] FRAPPE_API std::vector<std::string> get_user_groups(std::string_view user_name) noexcept;
[[nodiscard]] FRAPPE_API bool is_user_in_group(std::string_view user_name, std::string_view group_name) noexcept;

// Current user info
[[nodiscard]] FRAPPE_API std::uint32_t current_user_id() noexcept;
[[nodiscard]] FRAPPE_API std::uint32_t current_group_id() noexcept;
[[nodiscard]] FRAPPE_API std::string current_user_name() noexcept;
[[nodiscard]] FRAPPE_API std::string current_group_name() noexcept;
[[nodiscard]] FRAPPE_API std::vector<std::uint32_t> current_user_groups() noexcept;

// Cross-platform access rights
enum class access_rights : unsigned {
    none = 0,
    read = 1 << 0,
    write = 1 << 1,
    execute = 1 << 2,
    _delete = 1 << 3,
    list = 1 << 4,
    create = 1 << 5,
    all = read | write | execute | _delete | list | create
};

constexpr access_rights operator|(access_rights a, access_rights b) noexcept {
    return static_cast<access_rights>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}

constexpr access_rights operator&(access_rights a, access_rights b) noexcept {
    return static_cast<access_rights>(static_cast<unsigned>(a) & static_cast<unsigned>(b));
}

constexpr bool has_right(access_rights rights, access_rights flag) noexcept {
    return (static_cast<unsigned>(rights) & static_cast<unsigned>(flag)) != 0;
}

[[nodiscard]] FRAPPE_API result<bool> check_access(const path& p, access_rights rights) noexcept;
[[nodiscard]] FRAPPE_API result<access_rights> get_effective_rights(const path& p) noexcept;

// 4.7 Cross-platform access check for specific user
[[nodiscard]] FRAPPE_API result<bool> check_access(const path& p, std::string_view user_name, access_rights rights) noexcept;
[[nodiscard]] FRAPPE_API result<access_rights> get_effective_rights(const path& p, std::string_view user_name) noexcept;

// ============================================================================
// 4.4.1 Windows Security Identifier (SID) Support
// ============================================================================

enum class sid_type {
    unknown,
    user,
    group,
    domain,
    alias,              // Local group
    well_known_group,   // Everyone, Administrators, etc.
    deleted_account,
    invalid,
    computer,
    label               // Integrity label
};

struct security_identifier {
    std::string sid_string;     // "S-1-5-21-..."
    std::string domain;         // Domain name
    std::string account_name;   // Account name
    std::uint32_t rid = 0;      // Relative Identifier
    sid_type type = sid_type::unknown;
};

[[nodiscard]] FRAPPE_API std::string_view sid_type_name(sid_type type) noexcept;

// SID query functions
[[nodiscard]] FRAPPE_API result<security_identifier> get_sid(std::string_view account_name) noexcept;
[[nodiscard]] FRAPPE_API result<security_identifier> get_file_owner_sid(const path& p) noexcept;
[[nodiscard]] FRAPPE_API result<security_identifier> get_account_from_sid(std::string_view sid_string) noexcept;
[[nodiscard]] FRAPPE_API bool is_valid_sid(std::string_view sid_string) noexcept;

// Domain support
[[nodiscard]] FRAPPE_API std::string get_computer_name() noexcept;
[[nodiscard]] FRAPPE_API std::string get_domain_name() noexcept;
[[nodiscard]] FRAPPE_API bool is_domain_joined() noexcept;
[[nodiscard]] FRAPPE_API result<security_identifier> lookup_account(std::string_view domain, std::string_view name) noexcept;
[[nodiscard]] FRAPPE_API result<std::vector<std::string>> get_local_users() noexcept;
[[nodiscard]] FRAPPE_API result<std::vector<std::string>> get_local_groups() noexcept;

// Well-known SID constants
namespace well_known_sid {
    inline constexpr std::string_view everyone = "S-1-1-0";
    inline constexpr std::string_view local_system = "S-1-5-18";
    inline constexpr std::string_view local_service = "S-1-5-19";
    inline constexpr std::string_view network_service = "S-1-5-20";
    inline constexpr std::string_view authenticated_users = "S-1-5-11";
    inline constexpr std::string_view interactive = "S-1-5-4";
    inline constexpr std::string_view anonymous = "S-1-5-7";
}

// Well-known RID constants
namespace well_known_rid {
    inline constexpr std::uint32_t administrator = 500;
    inline constexpr std::uint32_t guest = 501;
    inline constexpr std::uint32_t admins = 544;
    inline constexpr std::uint32_t users = 545;
    inline constexpr std::uint32_t guests = 546;
    inline constexpr std::uint32_t power_users = 547;
}

// ============================================================================
// 4.5 ACL Support
// ============================================================================

enum class acl_type {
    access,      // Access ACL
    default_acl  // Default ACL (directory inheritance)
};

enum class acl_tag {
    user_obj,   // File owner
    user,       // Specific user
    group_obj,  // File group
    group,      // Specific group
    mask,       // Mask
    other       // Others
};

struct acl_entry {
    acl_tag tag = acl_tag::other;
    std::string qualifier;  // User/group name (empty for user_obj, group_obj, mask, other)
    bool read = false;
    bool write = false;
    bool execute = false;
};

struct acl_info {
    std::vector<acl_entry> entries;
    bool has_mask = false;
};

[[nodiscard]] FRAPPE_API result<acl_info> get_acl(const path& p) noexcept;
[[nodiscard]] FRAPPE_API result<acl_info> get_acl(const path& p, acl_type type) noexcept;
[[nodiscard]] FRAPPE_API result<acl_info> get_default_acl(const path& p) noexcept;
[[nodiscard]] FRAPPE_API result<bool> has_acl(const path& p) noexcept;
[[nodiscard]] FRAPPE_API result<bool> has_default_acl(const path& p) noexcept;
[[nodiscard]] FRAPPE_API result<bool> is_acl_supported(const path& p) noexcept;
[[nodiscard]] FRAPPE_API std::string acl_to_string(const acl_info& acl) noexcept;

[[nodiscard]] FRAPPE_API void_result set_acl(const path& p, const acl_info& acl) noexcept;
[[nodiscard]] FRAPPE_API void_result set_default_acl(const path& p, const acl_info& acl) noexcept;
[[nodiscard]] FRAPPE_API void_result add_acl_entry(const path& p, const acl_entry& entry) noexcept;
[[nodiscard]] FRAPPE_API void_result remove_acl_entry(const path& p, const acl_entry& entry) noexcept;
[[nodiscard]] FRAPPE_API void_result remove_acl(const path& p) noexcept;
[[nodiscard]] FRAPPE_API void_result remove_default_acl(const path& p) noexcept;

// ============================================================================
// 4.6 Windows Attributes and Security
// ============================================================================

enum class win_attributes : std::uint32_t {
    none = 0,
    readonly = 1 << 0,
    hidden = 1 << 1,
    system = 1 << 2,
    archive = 1 << 5,
    compressed = 1 << 11,
    encrypted = 1 << 14,
    sparse = 1 << 9,
    reparse_point = 1 << 10,
    offline = 1 << 12,
    not_indexed = 1 << 13,
    temporary = 1 << 8
};

constexpr win_attributes operator|(win_attributes a, win_attributes b) noexcept {
    return static_cast<win_attributes>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

constexpr win_attributes operator&(win_attributes a, win_attributes b) noexcept {
    return static_cast<win_attributes>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}

constexpr bool has_attribute(win_attributes attrs, win_attributes flag) noexcept {
    return (static_cast<std::uint32_t>(attrs) & static_cast<std::uint32_t>(flag)) != 0;
}

// Windows attribute queries
[[nodiscard]] FRAPPE_API result<win_attributes> get_win_attributes(const path& p) noexcept;
[[nodiscard]] FRAPPE_API result<bool> is_readonly(const path& p) noexcept;
[[nodiscard]] FRAPPE_API result<bool> is_hidden(const path& p) noexcept;
[[nodiscard]] FRAPPE_API result<bool> is_system_file(const path& p) noexcept;
[[nodiscard]] FRAPPE_API result<bool> is_archive(const path& p) noexcept;
[[nodiscard]] FRAPPE_API result<bool> is_compressed(const path& p) noexcept;
[[nodiscard]] FRAPPE_API result<bool> is_encrypted(const path& p) noexcept;
[[nodiscard]] FRAPPE_API result<bool> is_sparse(const path& p) noexcept;
[[nodiscard]] FRAPPE_API result<bool> is_reparse_point(const path& p) noexcept;

// Windows attribute modification
[[nodiscard]] FRAPPE_API void_result set_win_attributes(const path& p, win_attributes attrs) noexcept;
[[nodiscard]] FRAPPE_API void_result add_win_attribute(const path& p, win_attributes attr) noexcept;
[[nodiscard]] FRAPPE_API void_result remove_win_attribute(const path& p, win_attributes attr) noexcept;
[[nodiscard]] FRAPPE_API void_result set_readonly(const path& p, bool value) noexcept;
[[nodiscard]] FRAPPE_API void_result set_hidden(const path& p, bool value) noexcept;
[[nodiscard]] FRAPPE_API void_result set_system_file(const path& p, bool value) noexcept;
[[nodiscard]] FRAPPE_API void_result set_archive(const path& p, bool value) noexcept;

} // namespace frappe

#endif // FRAPPE_PERMISSIONS_HPP
