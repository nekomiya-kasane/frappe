#ifdef FRAPPE_PLATFORM_LINUX

#include "frappe/permissions.hpp"

#include <cstring>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace frappe::detail {

    result<owner_info> get_owner_impl(const path &p) noexcept {
        owner_info info;

        struct stat st;
        if (stat(p.c_str(), &st) != 0) {
            return std::unexpected(make_error(errno, std::system_category()));
        }

        info.uid = st.st_uid;
        info.gid = st.st_gid;

        struct passwd *pw = getpwuid(st.st_uid);
        if (pw) {
            info.user_name = pw->pw_name;
        }

        struct group *gr = getgrgid(st.st_gid);
        if (gr) {
            info.group_name = gr->gr_name;
        }

        return info;
    }

    void_result set_owner_impl(const path &p, std::uint32_t uid, std::uint32_t gid) noexcept {
        if (chown(p.c_str(), uid, gid) != 0) {
            return std::unexpected(make_error(errno, std::system_category()));
        }
        return {};
    }

    std::optional<std::uint32_t> user_id_from_name_impl(std::string_view name) noexcept {
        std::string name_str(name);
        struct passwd *pw = getpwnam(name_str.c_str());
        if (pw) {
            return pw->pw_uid;
        }
        return std::nullopt;
    }

    std::optional<std::string> user_name_from_id_impl(std::uint32_t uid) noexcept {
        struct passwd *pw = getpwuid(uid);
        if (pw) {
            return std::string(pw->pw_name);
        }
        return std::nullopt;
    }

    std::optional<std::uint32_t> group_id_from_name_impl(std::string_view name) noexcept {
        std::string name_str(name);
        struct group *gr = getgrnam(name_str.c_str());
        if (gr) {
            return gr->gr_gid;
        }
        return std::nullopt;
    }

    std::optional<std::string> group_name_from_id_impl(std::uint32_t gid) noexcept {
        struct group *gr = getgrgid(gid);
        if (gr) {
            return std::string(gr->gr_name);
        }
        return std::nullopt;
    }

    std::uint32_t current_user_id_impl() noexcept {
        return getuid();
    }

    std::uint32_t current_group_id_impl() noexcept {
        return getgid();
    }

    std::string current_user_name_impl() noexcept {
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            return std::string(pw->pw_name);
        }
        return "";
    }

    std::string current_group_name_impl() noexcept {
        struct group *gr = getgrgid(getgid());
        if (gr) {
            return std::string(gr->gr_name);
        }
        return "";
    }

    std::vector<std::uint32_t> current_user_groups_impl() noexcept {
        std::vector<std::uint32_t> groups;

        int ngroups = getgroups(0, nullptr);
        if (ngroups > 0) {
            std::vector<gid_t> gids(ngroups);
            if (getgroups(ngroups, gids.data()) != -1) {
                for (gid_t gid : gids) {
                    groups.push_back(gid);
                }
            }
        }

        return groups;
    }

    perms get_umask_impl() noexcept {
        mode_t old_mask = umask(0);
        umask(old_mask);
        return static_cast<perms>(old_mask);
    }

    void set_umask_impl(perms mask) noexcept {
        umask(static_cast<mode_t>(mask) & 0777);
    }

    result<bool> check_access_impl(const path &p, access_rights rights) noexcept {
        int mode = F_OK;
        if (has_right(rights, access_rights::read)) {
            mode |= R_OK;
        }
        if (has_right(rights, access_rights::write)) {
            mode |= W_OK;
        }
        if (has_right(rights, access_rights::execute)) {
            mode |= X_OK;
        }

        if (access(p.c_str(), mode) == 0) {
            return true;
        }
        if (errno == EACCES) {
            return false;
        }
        return std::unexpected(make_error(errno, std::system_category()));
    }

    result<access_rights> get_effective_rights_impl(const path &p) noexcept {
        access_rights result = access_rights::none;

        if (access(p.c_str(), R_OK) == 0) {
            result = result | access_rights::read;
        }
        if (access(p.c_str(), W_OK) == 0) {
            result = result | access_rights::write;
        }
        if (access(p.c_str(), X_OK) == 0) {
            result = result | access_rights::execute;
        }

        struct stat st;
        if (stat(p.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (has_right(result, access_rights::read) && has_right(result, access_rights::execute)) {
                    result = result | access_rights::list;
                }
                if (has_right(result, access_rights::write) && has_right(result, access_rights::execute)) {
                    result = result | access_rights::create;
                    result = result | access_rights::_delete;
                }
            } else {
                if (has_right(result, access_rights::write)) {
                    result = result | access_rights::_delete;
                }
            }
        }

        return result;
    }

    // 4.5 ACL Support (stub - requires libacl)
    result<acl_info> get_acl_impl(const path &p, acl_type type) noexcept {
        (void)p;
        (void)type;
        // Linux ACL requires libacl, returning empty for now
        return acl_info{};
    }

    void_result set_acl_impl(const path &p, const acl_info &acl, acl_type type) noexcept {
        (void)p;
        (void)acl;
        (void)type;
        return std::unexpected(make_error(std::errc::function_not_supported));
    }

    result<bool> has_acl_impl(const path &p) noexcept {
        (void)p;
        return false;
    }

    result<bool> is_acl_supported_impl(const path &p) noexcept {
        (void)p;
        return false;
    }

    // 4.6 Windows Attributes (not applicable on Linux)
    result<win_attributes> get_win_attributes_impl(const path &p) noexcept {
        (void)p;
        return win_attributes::none;
    }

    void_result set_win_attributes_impl(const path &p, win_attributes attrs) noexcept {
        (void)p;
        (void)attrs;
        return std::unexpected(make_error(std::errc::function_not_supported));
    }

    // 4.4.1 SID Support (Windows-specific, stubs for Linux)
    result<security_identifier> get_sid_impl(std::string_view account_name) noexcept {
        (void)account_name;
        return std::unexpected(make_error(std::errc::function_not_supported));
    }

    result<security_identifier> get_file_owner_sid_impl(const path &p) noexcept {
        (void)p;
        return std::unexpected(make_error(std::errc::function_not_supported));
    }

    result<security_identifier> get_account_from_sid_impl(std::string_view sid_string) noexcept {
        (void)sid_string;
        return std::unexpected(make_error(std::errc::function_not_supported));
    }

    bool is_valid_sid_impl(std::string_view sid_string) noexcept {
        (void)sid_string;
        return false;
    }

    std::string get_computer_name_impl() noexcept {
        char hostname[256] = {0};
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            return hostname;
        }
        return "";
    }

    std::string get_domain_name_impl() noexcept {
        return "";
    }

    bool is_domain_joined_impl() noexcept {
        return false;
    }

    result<security_identifier> lookup_account_impl(std::string_view domain, std::string_view name) noexcept {
        (void)domain;
        (void)name;
        return std::unexpected(make_error(std::errc::function_not_supported));
    }

    result<std::vector<std::string>> get_local_users_impl() noexcept {
        std::vector<std::string> users;
        setpwent();
        struct passwd *pw;
        while ((pw = getpwent()) != nullptr) {
            if (pw->pw_uid >= 1000 || pw->pw_uid == 0) {
                users.push_back(pw->pw_name);
            }
        }
        endpwent();
        return users;
    }

    result<std::vector<std::string>> get_local_groups_impl() noexcept {
        std::vector<std::string> groups;
        setgrent();
        struct group *gr;
        while ((gr = getgrent()) != nullptr) {
            groups.push_back(gr->gr_name);
        }
        endgrent();
        return groups;
    }

} // namespace frappe::detail

#endif // FRAPPE_PLATFORM_LINUX
