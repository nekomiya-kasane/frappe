#ifdef FRAPPE_PLATFORM_WINDOWS

#    include "frappe/permissions.hpp"

#    include <Lmcons.h>
#    include <Windows.h>
#    include <aclapi.h>
#    include <io.h>
#    include <lm.h>
#    include <sddl.h>

#    pragma comment(lib, "advapi32.lib")
#    pragma comment(lib, "netapi32.lib")

namespace frappe::detail {

    result<owner_info> get_owner_impl(const path &p) noexcept {
        owner_info info;

        PSID owner_sid = nullptr;
        PSECURITY_DESCRIPTOR sd = nullptr;

        DWORD res =
            GetNamedSecurityInfoW(p.c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION,
                                  &owner_sid, nullptr, nullptr, nullptr, &sd);

        if (res != ERROR_SUCCESS) {
            return std::unexpected(make_error(static_cast<int>(res), std::system_category()));
        }

        if (owner_sid) {
            wchar_t name[256] = {0};
            wchar_t domain[256] = {0};
            DWORD name_size = 256;
            DWORD domain_size = 256;
            SID_NAME_USE sid_type;

            if (LookupAccountSidW(nullptr, owner_sid, name, &name_size, domain, &domain_size, &sid_type)) {
                std::wstring wname(name);
                info.user_name = std::string(wname.begin(), wname.end());
            }

            LPWSTR sid_string = nullptr;
            if (ConvertSidToStringSidW(owner_sid, &sid_string)) {
                info.uid = 0;
                LocalFree(sid_string);
            }
        }

        if (sd) {
            LocalFree(sd);
        }

        return info;
    }

    void_result set_owner_impl(const path &p, std::uint32_t uid, std::uint32_t gid) noexcept {
        (void)p;
        (void)uid;
        (void)gid;
        return std::unexpected(make_error(std::errc::function_not_supported));
    }

    namespace {
        // Helper to get SID from account name
        std::vector<BYTE> get_sid_from_name(std::string_view name) {
            std::wstring wname(name.begin(), name.end());

            DWORD sid_size = 0;
            DWORD domain_size = 0;
            SID_NAME_USE sid_type;

            LookupAccountNameW(nullptr, wname.c_str(), nullptr, &sid_size, nullptr, &domain_size, &sid_type);
            if (sid_size == 0) {
                return {};
            }

            std::vector<BYTE> sid_buffer(sid_size);
            std::vector<wchar_t> domain_buffer(domain_size);

            if (LookupAccountNameW(nullptr, wname.c_str(), sid_buffer.data(), &sid_size, domain_buffer.data(),
                                   &domain_size, &sid_type)) {
                return sid_buffer;
            }
            return {};
        }

        // Helper to get account name from SID
        std::optional<std::string> get_name_from_sid(PSID sid) {
            wchar_t name[256] = {0};
            wchar_t domain[256] = {0};
            DWORD name_size = 256;
            DWORD domain_size = 256;
            SID_NAME_USE sid_type;

            if (LookupAccountSidW(nullptr, sid, name, &name_size, domain, &domain_size, &sid_type)) {
                std::wstring wname(name);
                return std::string(wname.begin(), wname.end());
            }
            return std::nullopt;
        }

        // Get RID (Relative Identifier) from SID as a pseudo-UID
        std::uint32_t get_rid_from_sid(PSID sid) {
            if (!IsValidSid(sid)) {
                return 0;
            }
            DWORD sub_auth_count = *GetSidSubAuthorityCount(sid);
            if (sub_auth_count > 0) {
                return *GetSidSubAuthority(sid, sub_auth_count - 1);
            }
            return 0;
        }
    } // namespace

    std::optional<std::uint32_t> user_id_from_name_impl(std::string_view name) noexcept {
        auto sid_buffer = get_sid_from_name(name);
        if (sid_buffer.empty()) {
            return std::nullopt;
        }

        PSID sid = reinterpret_cast<PSID>(sid_buffer.data());
        return get_rid_from_sid(sid);
    }

    std::optional<std::string> user_name_from_id_impl(std::uint32_t uid) noexcept {
        // On Windows, we can't directly look up by RID without knowing the domain SID
        // Return nullopt for now - would need more complex implementation
        (void)uid;
        return std::nullopt;
    }

    std::optional<std::uint32_t> group_id_from_name_impl(std::string_view name) noexcept {
        auto sid_buffer = get_sid_from_name(name);
        if (sid_buffer.empty()) {
            return std::nullopt;
        }

        PSID sid = reinterpret_cast<PSID>(sid_buffer.data());
        return get_rid_from_sid(sid);
    }

    std::optional<std::string> group_name_from_id_impl(std::uint32_t gid) noexcept {
        (void)gid;
        return std::nullopt;
    }

    std::uint32_t current_user_id_impl() noexcept {
        HANDLE token = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
            return 0;
        }

        DWORD size = 0;
        GetTokenInformation(token, TokenUser, nullptr, 0, &size);
        if (size == 0) {
            CloseHandle(token);
            return 0;
        }

        std::vector<BYTE> buffer(size);
        if (!GetTokenInformation(token, TokenUser, buffer.data(), size, &size)) {
            CloseHandle(token);
            return 0;
        }

        TOKEN_USER *user = reinterpret_cast<TOKEN_USER *>(buffer.data());
        std::uint32_t rid = get_rid_from_sid(user->User.Sid);

        CloseHandle(token);
        return rid;
    }

    std::uint32_t current_group_id_impl() noexcept {
        HANDLE token = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
            return 0;
        }

        DWORD size = 0;
        GetTokenInformation(token, TokenPrimaryGroup, nullptr, 0, &size);
        if (size == 0) {
            CloseHandle(token);
            return 0;
        }

        std::vector<BYTE> buffer(size);
        if (!GetTokenInformation(token, TokenPrimaryGroup, buffer.data(), size, &size)) {
            CloseHandle(token);
            return 0;
        }

        TOKEN_PRIMARY_GROUP *group = reinterpret_cast<TOKEN_PRIMARY_GROUP *>(buffer.data());
        std::uint32_t rid = get_rid_from_sid(group->PrimaryGroup);

        CloseHandle(token);
        return rid;
    }

    std::string current_user_name_impl() noexcept {
        wchar_t username[UNLEN + 1] = {0};
        DWORD size = UNLEN + 1;
        if (GetUserNameW(username, &size)) {
            std::wstring wname(username);
            return std::string(wname.begin(), wname.end());
        }
        return "";
    }

    std::string current_group_name_impl() noexcept {
        HANDLE token = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
            return "";
        }

        DWORD size = 0;
        GetTokenInformation(token, TokenPrimaryGroup, nullptr, 0, &size);
        if (size == 0) {
            CloseHandle(token);
            return "";
        }

        std::vector<BYTE> buffer(size);
        if (!GetTokenInformation(token, TokenPrimaryGroup, buffer.data(), size, &size)) {
            CloseHandle(token);
            return "";
        }

        TOKEN_PRIMARY_GROUP *group = reinterpret_cast<TOKEN_PRIMARY_GROUP *>(buffer.data());
        auto name = get_name_from_sid(group->PrimaryGroup);

        CloseHandle(token);
        return name.value_or("");
    }

    std::vector<std::uint32_t> current_user_groups_impl() noexcept {
        std::vector<std::uint32_t> result;

        HANDLE token = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
            return result;
        }

        DWORD size = 0;
        GetTokenInformation(token, TokenGroups, nullptr, 0, &size);
        if (size == 0) {
            CloseHandle(token);
            return result;
        }

        std::vector<BYTE> buffer(size);
        if (!GetTokenInformation(token, TokenGroups, buffer.data(), size, &size)) {
            CloseHandle(token);
            return result;
        }

        TOKEN_GROUPS *groups = reinterpret_cast<TOKEN_GROUPS *>(buffer.data());
        for (DWORD i = 0; i < groups->GroupCount; ++i) {
            if ((groups->Groups[i].Attributes & SE_GROUP_ENABLED) != 0) {
                result.push_back(get_rid_from_sid(groups->Groups[i].Sid));
            }
        }

        CloseHandle(token);
        return result;
    }

    perms get_umask_impl() noexcept {
        int old_mask = _umask(0);
        _umask(old_mask);
        return static_cast<perms>(old_mask);
    }

    void set_umask_impl(perms mask) noexcept {
        _umask(static_cast<int>(mask) & 0777);
    }

    result<bool> check_access_impl(const path &p, access_rights rights) noexcept {
        DWORD desired_access = 0;
        if (has_right(rights, access_rights::read)) {
            desired_access |= GENERIC_READ;
        }
        if (has_right(rights, access_rights::write)) {
            desired_access |= GENERIC_WRITE;
        }
        if (has_right(rights, access_rights::execute)) {
            desired_access |= GENERIC_EXECUTE;
        }
        if (has_right(rights, access_rights::_delete)) {
            desired_access |= DELETE;
        }

        HANDLE hFile = CreateFileW(p.c_str(), desired_access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

        if (hFile == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            if (error == ERROR_ACCESS_DENIED) {
                return false;
            }
            return std::unexpected(make_error(static_cast<int>(error), std::system_category()));
        }

        CloseHandle(hFile);
        return true;
    }

    result<access_rights> get_effective_rights_impl(const path &p) noexcept {
        access_rights result = access_rights::none;

        auto read_check = check_access_impl(p, access_rights::read);
        if (read_check && *read_check) {
            result = result | access_rights::read;
        }

        auto write_check = check_access_impl(p, access_rights::write);
        if (write_check && *write_check) {
            result = result | access_rights::write;
        }

        auto exec_check = check_access_impl(p, access_rights::execute);
        if (exec_check && *exec_check) {
            result = result | access_rights::execute;
        }

        auto del_check = check_access_impl(p, access_rights::_delete);
        if (del_check && *del_check) {
            result = result | access_rights::_delete;
        }

        std::error_code ec;
        if (std::filesystem::is_directory(p, ec)) {
            result = result | access_rights::list;
            if (has_right(result, access_rights::write)) {
                result = result | access_rights::create;
            }
        }

        return result;
    }

    // 4.5 ACL Support
    namespace {
        acl_entry parse_ace(ACCESS_ALLOWED_ACE *allowed_ace) {
            acl_entry entry;
            entry.tag = acl_tag::user;

            PSID sid = &allowed_ace->SidStart;
            wchar_t name[256] = {0};
            wchar_t domain[256] = {0};
            DWORD name_size = 256;
            DWORD domain_size = 256;
            SID_NAME_USE sid_type;

            if (LookupAccountSidW(nullptr, sid, name, &name_size, domain, &domain_size, &sid_type)) {
                std::wstring wname(name);
                entry.qualifier = std::string(wname.begin(), wname.end());
                if (sid_type == SidTypeGroup || sid_type == SidTypeWellKnownGroup) {
                    entry.tag = acl_tag::group;
                }
            }

            ACCESS_MASK mask = allowed_ace->Mask;
            entry.read = (mask & FILE_GENERIC_READ) != 0;
            entry.write = (mask & FILE_GENERIC_WRITE) != 0;
            entry.execute = (mask & FILE_GENERIC_EXECUTE) != 0;

            return entry;
        }

        void parse_dacl_entries(PACL dacl, acl_info &info) {
            ACL_SIZE_INFORMATION acl_size;
            if (!GetAclInformation(dacl, &acl_size, sizeof(acl_size), AclSizeInformation)) {
                return;
            }

            for (DWORD i = 0; i < acl_size.AceCount; ++i) {
                LPVOID ace = nullptr;
                if (!GetAce(dacl, i, &ace)) {
                    continue;
                }

                ACE_HEADER *header = static_cast<ACE_HEADER *>(ace);
                if (header->AceType != ACCESS_ALLOWED_ACE_TYPE) {
                    continue;
                }

                info.entries.push_back(parse_ace(static_cast<ACCESS_ALLOWED_ACE *>(ace)));
            }
        }
    } // namespace

    result<acl_info> get_acl_impl(const path &p, acl_type type) noexcept {
        (void)type;
        acl_info info;

        PACL dacl = nullptr;
        PSECURITY_DESCRIPTOR sd = nullptr;

        DWORD res = GetNamedSecurityInfoW(p.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &dacl,
                                          nullptr, &sd);

        if (res != ERROR_SUCCESS) {
            return std::unexpected(make_error(static_cast<int>(res), std::system_category()));
        }

        if (dacl) {
            parse_dacl_entries(dacl, info);
        }

        if (sd) {
            LocalFree(sd);
        }

        return info;
    }

    void_result set_acl_impl(const path &p, const acl_info &acl, acl_type type) noexcept {
        (void)type; // Windows doesn't have default ACL concept like POSIX

        if (acl.entries.empty()) {
            // Remove all ACEs - set empty DACL
            DWORD res = SetNamedSecurityInfoW(const_cast<wchar_t *>(p.c_str()), SE_FILE_OBJECT,
                                              DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, nullptr,
                                              nullptr, nullptr, nullptr);
            if (res != ERROR_SUCCESS) {
                return std::unexpected(make_error(static_cast<int>(res), std::system_category()));
            }
            return {};
        }

        // Calculate required ACL size
        DWORD acl_size = sizeof(ACL);
        std::vector<PSID> sids;
        sids.reserve(acl.entries.size());

        for (const auto &entry : acl.entries) {
            PSID sid = nullptr;
            if (!entry.qualifier.empty()) {
                std::wstring wname(entry.qualifier.begin(), entry.qualifier.end());
                DWORD sid_size = 0;
                DWORD domain_size = 0;
                SID_NAME_USE sid_type;

                LookupAccountNameW(nullptr, wname.c_str(), nullptr, &sid_size, nullptr, &domain_size, &sid_type);
                if (sid_size > 0) {
                    sid = static_cast<PSID>(LocalAlloc(LPTR, sid_size));
                    std::vector<wchar_t> domain(domain_size);
                    if (!LookupAccountNameW(nullptr, wname.c_str(), sid, &sid_size, domain.data(), &domain_size,
                                            &sid_type)) {
                        LocalFree(sid);
                        sid = nullptr;
                    }
                }
            }

            if (sid) {
                acl_size += sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(sid);
                sids.push_back(sid);
            }
        }

        if (sids.empty()) {
            return std::unexpected(make_error(std::errc::invalid_argument));
        }

        // Allocate and initialize ACL
        PACL new_acl = static_cast<PACL>(LocalAlloc(LPTR, acl_size));
        if (!new_acl) {
            for (auto sid : sids) {
                LocalFree(sid);
            }
            return std::unexpected(make_error(std::errc::not_enough_memory));
        }

        if (!InitializeAcl(new_acl, acl_size, ACL_REVISION)) {
            LocalFree(new_acl);
            for (auto sid : sids) {
                LocalFree(sid);
            }
            return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
        }

        // Add ACEs
        size_t sid_idx = 0;
        for (const auto &entry : acl.entries) {
            if (sid_idx >= sids.size()) {
                break;
            }
            PSID sid = sids[sid_idx++];
            if (!sid) {
                continue;
            }

            ACCESS_MASK mask = 0;
            if (entry.read) {
                mask |= FILE_GENERIC_READ;
            }
            if (entry.write) {
                mask |= FILE_GENERIC_WRITE;
            }
            if (entry.execute) {
                mask |= FILE_GENERIC_EXECUTE;
            }

            if (!AddAccessAllowedAce(new_acl, ACL_REVISION, mask, sid)) {
                // Continue even if one ACE fails
            }
        }

        // Apply ACL
        DWORD res = SetNamedSecurityInfoW(const_cast<wchar_t *>(p.c_str()), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                          nullptr, nullptr, new_acl, nullptr);

        LocalFree(new_acl);
        for (auto sid : sids) {
            LocalFree(sid);
        }

        if (res != ERROR_SUCCESS) {
            return std::unexpected(make_error(static_cast<int>(res), std::system_category()));
        }
        return {};
    }

    result<bool> has_acl_impl(const path &p) noexcept {
        auto acl = get_acl_impl(p, acl_type::access);
        if (!acl) {
            return std::unexpected(acl.error());
        }
        return !acl->entries.empty();
    }

    result<bool> is_acl_supported_impl(const path &p) noexcept {
        (void)p;
        return true; // Windows always supports ACLs on NTFS
    }

    // 4.6 Windows Attributes
    result<win_attributes> get_win_attributes_impl(const path &p) noexcept {
        DWORD attrs = GetFileAttributesW(p.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) {
            return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
        }
        return static_cast<win_attributes>(attrs);
    }

    void_result set_win_attributes_impl(const path &p, win_attributes attrs) noexcept {
        if (!SetFileAttributesW(p.c_str(), static_cast<DWORD>(attrs))) {
            return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
        }
        return {};
    }

    // ============================================================================
    // 4.4.1 Windows Security Identifier (SID) Support
    // ============================================================================

    namespace {
        sid_type convert_sid_name_use(SID_NAME_USE use) {
            switch (use) {
            case SidTypeUser:
                return sid_type::user;
            case SidTypeGroup:
                return sid_type::group;
            case SidTypeDomain:
                return sid_type::domain;
            case SidTypeAlias:
                return sid_type::alias;
            case SidTypeWellKnownGroup:
                return sid_type::well_known_group;
            case SidTypeDeletedAccount:
                return sid_type::deleted_account;
            case SidTypeInvalid:
                return sid_type::invalid;
            case SidTypeComputer:
                return sid_type::computer;
            case SidTypeLabel:
                return sid_type::label;
            default:
                return sid_type::unknown;
            }
        }
    } // namespace

    result<security_identifier> get_sid_impl(std::string_view account_name) noexcept {
        security_identifier result;

        auto sid_buffer = get_sid_from_name(account_name);
        if (sid_buffer.empty()) {
            return std::unexpected(make_error(std::errc::invalid_argument));
        }

        PSID sid = reinterpret_cast<PSID>(sid_buffer.data());

        // Get SID string
        LPWSTR sid_string = nullptr;
        if (ConvertSidToStringSidW(sid, &sid_string)) {
            std::wstring wsid(sid_string);
            result.sid_string = std::string(wsid.begin(), wsid.end());
            LocalFree(sid_string);
        }

        // Get account info
        wchar_t name[256] = {0};
        wchar_t domain[256] = {0};
        DWORD name_size = 256;
        DWORD domain_size = 256;
        SID_NAME_USE sid_use;

        if (LookupAccountSidW(nullptr, sid, name, &name_size, domain, &domain_size, &sid_use)) {
            std::wstring wname(name);
            std::wstring wdomain(domain);
            result.account_name = std::string(wname.begin(), wname.end());
            result.domain = std::string(wdomain.begin(), wdomain.end());
            result.type = convert_sid_name_use(sid_use);
        }

        result.rid = get_rid_from_sid(sid);

        return result;
    }

    result<security_identifier> get_file_owner_sid_impl(const path &p) noexcept {
        security_identifier result;

        PSID owner_sid = nullptr;
        PSECURITY_DESCRIPTOR sd = nullptr;

        DWORD res = GetNamedSecurityInfoW(p.c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &owner_sid, nullptr,
                                          nullptr, nullptr, &sd);

        if (res != ERROR_SUCCESS) {
            return std::unexpected(make_error(static_cast<int>(res), std::system_category()));
        }

        if (owner_sid) {
            // Get SID string
            LPWSTR sid_string = nullptr;
            if (ConvertSidToStringSidW(owner_sid, &sid_string)) {
                std::wstring wsid(sid_string);
                result.sid_string = std::string(wsid.begin(), wsid.end());
                LocalFree(sid_string);
            }

            // Get account info
            wchar_t name[256] = {0};
            wchar_t domain[256] = {0};
            DWORD name_size = 256;
            DWORD domain_size = 256;
            SID_NAME_USE sid_use;

            if (LookupAccountSidW(nullptr, owner_sid, name, &name_size, domain, &domain_size, &sid_use)) {
                std::wstring wname(name);
                std::wstring wdomain(domain);
                result.account_name = std::string(wname.begin(), wname.end());
                result.domain = std::string(wdomain.begin(), wdomain.end());
                result.type = convert_sid_name_use(sid_use);
            }

            result.rid = get_rid_from_sid(owner_sid);
        }

        if (sd) {
            LocalFree(sd);
        }

        return result;
    }

    result<security_identifier> get_account_from_sid_impl(std::string_view sid_string) noexcept {
        security_identifier result;
        result.sid_string = std::string(sid_string);

        std::wstring wsid(sid_string.begin(), sid_string.end());
        PSID sid = nullptr;

        if (!ConvertStringSidToSidW(wsid.c_str(), &sid)) {
            return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
        }

        wchar_t name[256] = {0};
        wchar_t domain[256] = {0};
        DWORD name_size = 256;
        DWORD domain_size = 256;
        SID_NAME_USE sid_use;

        if (LookupAccountSidW(nullptr, sid, name, &name_size, domain, &domain_size, &sid_use)) {
            std::wstring wname(name);
            std::wstring wdomain(domain);
            result.account_name = std::string(wname.begin(), wname.end());
            result.domain = std::string(wdomain.begin(), wdomain.end());
            result.type = convert_sid_name_use(sid_use);
        }

        result.rid = get_rid_from_sid(sid);

        LocalFree(sid);
        return result;
    }

    bool is_valid_sid_impl(std::string_view sid_string) noexcept {
        std::wstring wsid(sid_string.begin(), sid_string.end());
        PSID sid = nullptr;

        if (!ConvertStringSidToSidW(wsid.c_str(), &sid)) {
            return false;
        }

        bool valid = IsValidSid(sid) != FALSE;
        LocalFree(sid);
        return valid;
    }

    std::string get_computer_name_impl() noexcept {
        wchar_t buffer[MAX_COMPUTERNAME_LENGTH + 1] = {0};
        DWORD size = MAX_COMPUTERNAME_LENGTH + 1;

        if (GetComputerNameW(buffer, &size)) {
            std::wstring wname(buffer);
            return std::string(wname.begin(), wname.end());
        }
        return "";
    }

    std::string get_domain_name_impl() noexcept {
        LPWSTR name_buffer = nullptr;
        NETSETUP_JOIN_STATUS join_status;

        if (NetGetJoinInformation(nullptr, &name_buffer, &join_status) == NERR_Success) {
            std::wstring wname(name_buffer);
            NetApiBufferFree(name_buffer);

            if (join_status == NetSetupDomainName) {
                return std::string(wname.begin(), wname.end());
            }
        }
        return "";
    }

    bool is_domain_joined_impl() noexcept {
        LPWSTR name_buffer = nullptr;
        NETSETUP_JOIN_STATUS join_status;

        if (NetGetJoinInformation(nullptr, &name_buffer, &join_status) == NERR_Success) {
            NetApiBufferFree(name_buffer);
            return join_status == NetSetupDomainName;
        }
        return false;
    }

    result<security_identifier> lookup_account_impl(std::string_view domain, std::string_view name) noexcept {
        std::string full_name;
        if (!domain.empty()) {
            full_name = std::string(domain) + "\\" + std::string(name);
        } else {
            full_name = std::string(name);
        }

        return get_sid_impl(full_name);
    }

    result<std::vector<std::string>> get_local_users_impl() noexcept {
        std::vector<std::string> users;

        LPUSER_INFO_0 buffer = nullptr;
        DWORD entries_read = 0;
        DWORD total_entries = 0;

        NET_API_STATUS status = NetUserEnum(nullptr, // Local computer
                                            0,       // Level 0 - just names
                                            FILTER_NORMAL_ACCOUNT, reinterpret_cast<LPBYTE *>(&buffer),
                                            MAX_PREFERRED_LENGTH, &entries_read, &total_entries, nullptr);

        if (status == NERR_Success || status == ERROR_MORE_DATA) {
            for (DWORD i = 0; i < entries_read; ++i) {
                std::wstring wname(buffer[i].usri0_name);
                users.push_back(std::string(wname.begin(), wname.end()));
            }
            NetApiBufferFree(buffer);
        } else {
            return std::unexpected(make_error(static_cast<int>(status), std::system_category()));
        }

        return users;
    }

    result<std::vector<std::string>> get_local_groups_impl() noexcept {
        std::vector<std::string> groups;

        LPLOCALGROUP_INFO_0 buffer = nullptr;
        DWORD entries_read = 0;
        DWORD total_entries = 0;

        NET_API_STATUS status = NetLocalGroupEnum(nullptr, // Local computer
                                                  0,       // Level 0 - just names
                                                  reinterpret_cast<LPBYTE *>(&buffer), MAX_PREFERRED_LENGTH,
                                                  &entries_read, &total_entries, nullptr);

        if (status == NERR_Success || status == ERROR_MORE_DATA) {
            for (DWORD i = 0; i < entries_read; ++i) {
                std::wstring wname(buffer[i].lgrpi0_name);
                groups.push_back(std::string(wname.begin(), wname.end()));
            }
            NetApiBufferFree(buffer);
        } else {
            return std::unexpected(make_error(static_cast<int>(status), std::system_category()));
        }

        return groups;
    }

} // namespace frappe::detail

#endif // FRAPPE_PLATFORM_WINDOWS
