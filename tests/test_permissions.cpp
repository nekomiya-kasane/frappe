#include <gtest/gtest.h>
#include "frappe/permissions.hpp"
#include <filesystem>
#include <fstream>

class PermissionsTest : public ::testing::Test {
protected:
    void SetUp() override {
        _test_dir = std::filesystem::temp_directory_path() / "frappe_permissions_test";
        std::filesystem::create_directories(_test_dir);
        
        _test_file = _test_dir / "test_file.txt";
        std::ofstream(_test_file) << "test content";
        
        _test_subdir = _test_dir / "subdir";
        std::filesystem::create_directories(_test_subdir);
    }
    
    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(_test_dir, ec);
    }
    
    std::filesystem::path _test_dir;
    std::filesystem::path _test_file;
    std::filesystem::path _test_subdir;
};

TEST_F(PermissionsTest, GetPermissions) {
    auto perms = frappe::get_permissions(_test_file);
    EXPECT_TRUE(perms.has_value());
    EXPECT_NE(*perms, frappe::perms::none);
}

TEST_F(PermissionsTest, HasPermission) {
    auto r = frappe::has_permission(_test_file, frappe::perms::owner_read);
    EXPECT_TRUE(r.has_value() && *r);
}

TEST_F(PermissionsTest, IsReadable) {
    auto r = frappe::is_readable(_test_file);
    EXPECT_TRUE(r.has_value() && *r);
}

TEST_F(PermissionsTest, IsWritable) {
    auto r = frappe::is_writable(_test_file);
    EXPECT_TRUE(r.has_value() && *r);
}

TEST_F(PermissionsTest, PermissionsToString) {
    auto str = frappe::permissions_to_string(
        frappe::perms::owner_read | frappe::perms::owner_write | frappe::perms::owner_exec |
        frappe::perms::group_read | frappe::perms::group_exec |
        frappe::perms::others_read | frappe::perms::others_exec
    );
    EXPECT_EQ(str, "rwxr-xr-x");
}

TEST_F(PermissionsTest, PermissionsToStringAllDash) {
    auto str = frappe::permissions_to_string(frappe::perms::none);
    EXPECT_EQ(str, "---------");
}

TEST_F(PermissionsTest, PermissionsToOctal) {
    auto octal = frappe::permissions_to_octal(
        frappe::perms::owner_read | frappe::perms::owner_write | frappe::perms::owner_exec |
        frappe::perms::group_read | frappe::perms::group_exec |
        frappe::perms::others_read | frappe::perms::others_exec
    );
    EXPECT_EQ(octal, "0755");
}

TEST_F(PermissionsTest, PermissionsFromString) {
    auto perms = frappe::permissions_from_string("rwxr-xr-x");
    EXPECT_TRUE((perms & frappe::perms::owner_read) != frappe::perms::none);
    EXPECT_TRUE((perms & frappe::perms::owner_write) != frappe::perms::none);
    EXPECT_TRUE((perms & frappe::perms::owner_exec) != frappe::perms::none);
    EXPECT_TRUE((perms & frappe::perms::group_read) != frappe::perms::none);
    EXPECT_TRUE((perms & frappe::perms::group_exec) != frappe::perms::none);
    EXPECT_TRUE((perms & frappe::perms::others_read) != frappe::perms::none);
    EXPECT_TRUE((perms & frappe::perms::others_exec) != frappe::perms::none);
}

TEST_F(PermissionsTest, PermissionsFromOctal) {
    auto perms = frappe::permissions_from_octal("755");
    EXPECT_TRUE((perms & frappe::perms::owner_read) != frappe::perms::none);
    EXPECT_TRUE((perms & frappe::perms::owner_write) != frappe::perms::none);
    EXPECT_TRUE((perms & frappe::perms::owner_exec) != frappe::perms::none);
}

TEST_F(PermissionsTest, PermissionsFromMode) {
    auto perms = frappe::permissions_from_mode(0755);
    EXPECT_TRUE((perms & frappe::perms::owner_read) != frappe::perms::none);
    EXPECT_TRUE((perms & frappe::perms::owner_write) != frappe::perms::none);
    EXPECT_TRUE((perms & frappe::perms::owner_exec) != frappe::perms::none);
}

TEST_F(PermissionsTest, GetOwner) {
    auto owner = frappe::get_owner(_test_file);
    EXPECT_TRUE(owner.has_value());
}

TEST_F(PermissionsTest, CurrentUserName) {
    auto name = frappe::current_user_name();
    EXPECT_FALSE(name.empty());
}

TEST_F(PermissionsTest, CheckAccess) {
    auto r = frappe::check_access(_test_file, frappe::access_rights::read);
    EXPECT_TRUE(r.has_value() && *r);
}

TEST_F(PermissionsTest, GetEffectiveRights) {
    auto rights = frappe::get_effective_rights(_test_file);
    EXPECT_TRUE(rights.has_value());
    EXPECT_TRUE(frappe::has_right(*rights, frappe::access_rights::read));
}

TEST_F(PermissionsTest, AccessRightsOperators) {
    auto rights = frappe::access_rights::read | frappe::access_rights::write;
    EXPECT_TRUE(frappe::has_right(rights, frappe::access_rights::read));
    EXPECT_TRUE(frappe::has_right(rights, frappe::access_rights::write));
    EXPECT_FALSE(frappe::has_right(rights, frappe::access_rights::execute));
}

TEST_F(PermissionsTest, PermissionsStringRoundTrip) {
    auto original = frappe::perms::owner_read | frappe::perms::owner_write | 
                    frappe::perms::group_read | frappe::perms::others_read;
    auto str = frappe::permissions_to_string(original);
    auto parsed = frappe::permissions_from_string(str);
    
    EXPECT_EQ((original & frappe::perms::mask), (parsed & frappe::perms::mask));
}

TEST_F(PermissionsTest, PermissionsOctalRoundTrip) {
    auto original = frappe::perms::owner_read | frappe::perms::owner_write | frappe::perms::owner_exec |
                    frappe::perms::group_read | frappe::perms::group_exec |
                    frappe::perms::others_read | frappe::perms::others_exec;
    auto octal = frappe::permissions_to_octal(original);
    auto parsed = frappe::permissions_from_octal(octal);
    
    EXPECT_EQ((original & frappe::perms::mask), (parsed & frappe::perms::mask));
}

// ============================================================================
// 4.4 User/Group existence and membership tests
// ============================================================================

TEST_F(PermissionsTest, UserExists) {
    auto current = frappe::current_user_name();
    EXPECT_TRUE(frappe::user_exists(current));
    EXPECT_FALSE(frappe::user_exists("nonexistent_user_xyz123"));
}

TEST_F(PermissionsTest, GroupExists) {
    // On Windows, group_exists may return false for all
    auto result = frappe::group_exists("nonexistent_group_xyz123");
    EXPECT_FALSE(result);
}

TEST_F(PermissionsTest, GetUserGroups) {
    auto current = frappe::current_user_name();
    auto groups = frappe::get_user_groups(current);
    // May be empty on some systems
}

// ============================================================================
// 4.5 ACL tests
// ============================================================================

TEST_F(PermissionsTest, GetAcl) {
    auto acl = frappe::get_acl(_test_file);
    EXPECT_TRUE(acl.has_value());
}

TEST_F(PermissionsTest, HasAcl) {
    auto has = frappe::has_acl(_test_file);
    EXPECT_TRUE(has.has_value());
}

TEST_F(PermissionsTest, IsAclSupported) {
    auto supported = frappe::is_acl_supported(_test_file);
    EXPECT_TRUE(supported.has_value());
}

TEST_F(PermissionsTest, AclToString) {
    frappe::acl_info acl;
    frappe::acl_entry entry;
    entry.tag = frappe::acl_tag::user_obj;
    entry.read = true;
    entry.write = true;
    entry.execute = false;
    acl.entries.push_back(entry);
    
    auto str = frappe::acl_to_string(acl);
    EXPECT_FALSE(str.empty());
    EXPECT_NE(str.find("user::"), std::string::npos);
}

// ============================================================================
// 4.6 Windows attributes tests
// ============================================================================

TEST_F(PermissionsTest, GetWinAttributes) {
    auto attrs = frappe::get_win_attributes(_test_file);
    EXPECT_TRUE(attrs.has_value());
}

TEST_F(PermissionsTest, IsReadonly) {
    auto readonly = frappe::is_readonly(_test_file);
    EXPECT_TRUE(readonly.has_value());
    EXPECT_FALSE(*readonly);  // New file should not be readonly
}

TEST_F(PermissionsTest, IsHidden) {
    auto hidden = frappe::is_hidden(_test_file);
    EXPECT_TRUE(hidden.has_value());
    EXPECT_FALSE(*hidden);  // New file should not be hidden
}

TEST_F(PermissionsTest, IsArchive) {
    auto archive = frappe::is_archive(_test_file);
    EXPECT_TRUE(archive.has_value());
    // New files typically have archive bit set on Windows
}

#ifdef _WIN32
TEST_F(PermissionsTest, SetReadonly) {
    auto result = frappe::set_readonly(_test_file, true);
    EXPECT_TRUE(result.has_value());
    
    auto readonly = frappe::is_readonly(_test_file);
    EXPECT_TRUE(readonly.has_value() && *readonly);
    
    // Reset
    frappe::set_readonly(_test_file, false);
}

TEST_F(PermissionsTest, SetHidden) {
    auto result = frappe::set_hidden(_test_file, true);
    EXPECT_TRUE(result.has_value());
    
    auto hidden = frappe::is_hidden(_test_file);
    EXPECT_TRUE(hidden.has_value() && *hidden);
    
    // Reset
    frappe::set_hidden(_test_file, false);
}

TEST_F(PermissionsTest, WinAttributeOperators) {
    auto attrs = frappe::win_attributes::readonly | frappe::win_attributes::hidden;
    EXPECT_TRUE(frappe::has_attribute(attrs, frappe::win_attributes::readonly));
    EXPECT_TRUE(frappe::has_attribute(attrs, frappe::win_attributes::hidden));
    EXPECT_FALSE(frappe::has_attribute(attrs, frappe::win_attributes::system));
}
#endif

// ============================================================================
// 4.7 Cross-platform access check for specific user
// ============================================================================

TEST_F(PermissionsTest, CheckAccessForCurrentUser) {
    auto current = frappe::current_user_name();
    auto result = frappe::check_access(_test_file, current, frappe::access_rights::read);
    EXPECT_TRUE(result.has_value() && *result);
}

TEST_F(PermissionsTest, GetEffectiveRightsForCurrentUser) {
    auto current = frappe::current_user_name();
    auto rights = frappe::get_effective_rights(_test_file, current);
    EXPECT_TRUE(rights.has_value());
    EXPECT_TRUE(frappe::has_right(*rights, frappe::access_rights::read));
}

// ============================================================================
// 4.4.1 Windows SID and Domain tests
// ============================================================================

TEST_F(PermissionsTest, SidTypeName) {
    EXPECT_EQ(frappe::sid_type_name(frappe::sid_type::user), "user");
    EXPECT_EQ(frappe::sid_type_name(frappe::sid_type::group), "group");
    EXPECT_EQ(frappe::sid_type_name(frappe::sid_type::well_known_group), "well_known_group");
}

TEST_F(PermissionsTest, GetComputerName) {
    auto name = frappe::get_computer_name();
    EXPECT_FALSE(name.empty());
}

#ifdef _WIN32
TEST_F(PermissionsTest, GetSidFromCurrentUser) {
    auto current = frappe::current_user_name();
    auto sid = frappe::get_sid(current);
    EXPECT_TRUE(sid.has_value());
    EXPECT_FALSE(sid->sid_string.empty());
    EXPECT_EQ(sid->account_name, current);
    EXPECT_GT(sid->rid, 0u);
}

TEST_F(PermissionsTest, GetFileOwnerSid) {
    auto sid = frappe::get_file_owner_sid(_test_file);
    EXPECT_TRUE(sid.has_value());
    EXPECT_FALSE(sid->sid_string.empty());
    EXPECT_FALSE(sid->account_name.empty());
}

TEST_F(PermissionsTest, GetAccountFromSid) {
    // Get current user's SID first
    auto current = frappe::current_user_name();
    auto sid = frappe::get_sid(current);
    ASSERT_TRUE(sid.has_value());
    
    // Now look up account from SID string
    auto account = frappe::get_account_from_sid(sid->sid_string);
    EXPECT_TRUE(account.has_value());
    EXPECT_EQ(account->account_name, current);
}

TEST_F(PermissionsTest, IsValidSid) {
    EXPECT_TRUE(frappe::is_valid_sid("S-1-1-0"));  // Everyone
    EXPECT_TRUE(frappe::is_valid_sid("S-1-5-18")); // SYSTEM
    EXPECT_FALSE(frappe::is_valid_sid("invalid"));
    EXPECT_FALSE(frappe::is_valid_sid(""));
}

TEST_F(PermissionsTest, WellKnownSids) {
    auto everyone = frappe::get_account_from_sid(std::string(frappe::well_known_sid::everyone));
    EXPECT_TRUE(everyone.has_value());
    EXPECT_EQ(everyone->type, frappe::sid_type::well_known_group);
    
    auto system = frappe::get_account_from_sid(std::string(frappe::well_known_sid::local_system));
    EXPECT_TRUE(system.has_value());
}

TEST_F(PermissionsTest, IsDomainJoined) {
    // Just test that it doesn't crash
    auto joined = frappe::is_domain_joined();
    (void)joined;
}

TEST_F(PermissionsTest, GetDomainName) {
    // Just test that it doesn't crash
    auto domain = frappe::get_domain_name();
    // May be empty if not domain-joined
}

TEST_F(PermissionsTest, GetLocalUsers) {
    auto users = frappe::get_local_users();
    EXPECT_TRUE(users.has_value());
    EXPECT_GT(users->size(), 0u);
    
    // Current user should be in the list
    auto current = frappe::current_user_name();
    bool found = false;
    for (const auto& u : *users) {
        if (u == current) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(PermissionsTest, GetLocalGroups) {
    auto groups = frappe::get_local_groups();
    EXPECT_TRUE(groups.has_value());
    EXPECT_GT(groups->size(), 0u);
}

TEST_F(PermissionsTest, LookupAccount) {
    auto current = frappe::current_user_name();
    auto computer = frappe::get_computer_name();
    
    auto sid = frappe::lookup_account(computer, current);
    EXPECT_TRUE(sid.has_value());
    EXPECT_EQ(sid->account_name, current);
}

TEST_F(PermissionsTest, WellKnownRids) {
    EXPECT_EQ(frappe::well_known_rid::administrator, 500u);
    EXPECT_EQ(frappe::well_known_rid::guest, 501u);
    EXPECT_EQ(frappe::well_known_rid::admins, 544u);
    EXPECT_EQ(frappe::well_known_rid::users, 545u);
}
#endif
