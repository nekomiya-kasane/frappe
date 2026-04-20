#include "frappe/formatters.hpp"

#include <format>
#include <gtest/gtest.h>

using namespace frappe;

namespace {

file_entry make_entry(const std::string &name, file_type type, std::uintmax_t size = 0) {
    file_entry e;
    e.file_path = "/tmp/" + name;
    e.name = name;
    e.stem = std::filesystem::path(name).stem().string();
    e.extension = std::filesystem::path(name).extension().string();
    e.type = type;
    e.size = size;
    e.is_symlink = false;
    e.is_broken_link = false;
    e.has_acl = false;
    e.has_xattr = true;
    e.permissions = perms::owner_read | perms::owner_write | perms::group_read;
    e.hard_link_count = 2;
    e.inode = 12345;
    e.uid = 1000;
    e.gid = 1000;
    e.owner = "alice";
    e.group = "staff";
    return e;
}

} // namespace

// ── file_type formatter ─────────────────────────────────────────────────

TEST(FormattersTest, FileTypeDefault) {
    auto s = std::format("{}", file_type::regular);
    EXPECT_FALSE(s.empty());
}

TEST(FormattersTest, FileTypeChar) {
    auto s = std::format("{:c}", file_type::directory);
    EXPECT_EQ(s.size(), 1u);
}

TEST(FormattersTest, FileTypeCharRegular) {
    auto s = std::format("{:c}", file_type::regular);
    EXPECT_EQ(s.size(), 1u);
}

// ── perms formatter ─────────────────────────────────────────────────────

TEST(FormattersTest, PermsDefault) {
    auto s = std::format("{}", perms::owner_read | perms::owner_write);
    EXPECT_FALSE(s.empty());
}

TEST(FormattersTest, PermsOctal) {
    auto s = std::format("{:o}", perms::owner_read | perms::owner_write | perms::owner_exec);
    EXPECT_FALSE(s.empty());
}

TEST(FormattersTest, PermsString) {
    auto s = std::format("{:s}", perms::owner_read | perms::owner_write);
    EXPECT_FALSE(s.empty());
}

// ── filesystem_type formatter ───────────────────────────────────────────

TEST(FormattersTest, FilesystemType) {
    auto s = std::format("{}", filesystem_type::unknown);
    EXPECT_FALSE(s.empty());
}

// ── file_id formatter ───────────────────────────────────────────────────

TEST(FormattersTest, FileId) {
    file_id id{42, 100};
    auto s = std::format("{}", id);
    EXPECT_EQ(s, "42:100");
}

// ── link_type formatter ─────────────────────────────────────────────────

TEST(FormattersTest, LinkTypeNone) {
    auto s = std::format("{}", link_type::none);
    EXPECT_EQ(s, "none");
}

TEST(FormattersTest, LinkTypeSymlink) {
    auto s = std::format("{}", link_type::symlink);
    EXPECT_EQ(s, "symlink");
}

TEST(FormattersTest, LinkTypeHardlink) {
    auto s = std::format("{}", link_type::hardlink);
    EXPECT_EQ(s, "hardlink");
}

// ── link_info formatter ─────────────────────────────────────────────────

TEST(FormattersTest, LinkInfoRegular) {
    link_info info;
    info.type = link_type::none;
    auto s = std::format("{}", info);
    EXPECT_EQ(s, "regular");
}

TEST(FormattersTest, LinkInfoHardlink) {
    link_info info;
    info.type = link_type::hardlink;
    info.link_count = 3;
    auto s = std::format("{}", info);
    EXPECT_EQ(s, "hardlink (count=3)");
}

TEST(FormattersTest, LinkInfoSymlink) {
    link_info info;
    info.type = link_type::symlink;
    info.target = "/usr/bin/python";
    info.is_broken = false;
    info.is_circular = false;
    auto s = std::format("{}", info);
    EXPECT_TRUE(s.find("symlink") != std::string::npos);
    EXPECT_TRUE(s.find("/usr/bin/python") != std::string::npos);
}

TEST(FormattersTest, LinkInfoBrokenSymlink) {
    link_info info;
    info.type = link_type::symlink;
    info.target = "/missing";
    info.is_broken = true;
    info.is_circular = false;
    auto s = std::format("{}", info);
    EXPECT_TRUE(s.find("[broken]") != std::string::npos);
}

// ── owner_info formatter ────────────────────────────────────────────────

TEST(FormattersTest, OwnerInfoBoth) {
    owner_info info;
    info.user_name = "alice";
    info.group_name = "staff";
    auto s = std::format("{}", info);
    EXPECT_EQ(s, "alice:staff");
}

TEST(FormattersTest, OwnerInfoUserOnly) {
    owner_info info;
    info.user_name = "alice";
    info.group_name = "staff";
    auto s = std::format("{:u}", info);
    EXPECT_EQ(s, "alice");
}

TEST(FormattersTest, OwnerInfoGroupOnly) {
    owner_info info;
    info.user_name = "alice";
    info.group_name = "staff";
    auto s = std::format("{:g}", info);
    EXPECT_EQ(s, "staff");
}

// ── file_entry formatter ────────────────────────────────────────────────

TEST(FormattersTest, FileEntryDefaultName) {
    auto e = make_entry("test.txt", file_type::regular, 1024);
    auto s = std::format("{}", e);
    EXPECT_EQ(s, "test.txt");
}

TEST(FormattersTest, FileEntryNameField) {
    auto e = make_entry("test.txt", file_type::regular, 1024);
    auto s = std::format("{:n}", e);
    EXPECT_EQ(s, "test.txt");
}

TEST(FormattersTest, FileEntryPathField) {
    auto e = make_entry("test.txt", file_type::regular, 1024);
    auto s = std::format("{:p}", e);
    EXPECT_TRUE(s.find("test.txt") != std::string::npos);
}

TEST(FormattersTest, FileEntrySizeField) {
    auto e = make_entry("test.txt", file_type::regular, 4096);
    auto s = std::format("{:s}", e);
    EXPECT_EQ(s, "4096");
}

TEST(FormattersTest, FileEntrySizeHuman) {
    auto e = make_entry("test.txt", file_type::regular, 1048576);
    auto s = std::format("{:s:h}", e);
    EXPECT_TRUE(s.find("M") != std::string::npos || s.find("1") != std::string::npos);
}

TEST(FormattersTest, FileEntryMultipleFields) {
    auto e = make_entry("test.txt", file_type::regular, 1024);
    auto s = std::format("{:sn}", e);
    EXPECT_TRUE(s.find("1024") != std::string::npos);
    EXPECT_TRUE(s.find("test.txt") != std::string::npos);
}

TEST(FormattersTest, FileEntryOwnerGroup) {
    auto e = make_entry("test.txt", file_type::regular, 1024);
    auto s = std::format("{:og}", e);
    EXPECT_TRUE(s.find("alice") != std::string::npos);
    EXPECT_TRUE(s.find("staff") != std::string::npos);
}

TEST(FormattersTest, FileEntryInode) {
    auto e = make_entry("test.txt", file_type::regular);
    auto s = std::format("{:i}", e);
    EXPECT_EQ(s, "12345");
}

TEST(FormattersTest, FileEntryLinkCount) {
    auto e = make_entry("test.txt", file_type::regular);
    auto s = std::format("{:l}", e);
    EXPECT_EQ(s, "2");
}

TEST(FormattersTest, FileEntryAclIndicator) {
    auto e = make_entry("test.txt", file_type::regular);
    auto s = std::format("{:+}", e);
    EXPECT_EQ(s, " "); // has_acl = false
}

TEST(FormattersTest, FileEntryXattrIndicator) {
    auto e = make_entry("test.txt", file_type::regular);
    auto s = std::format("{:@}", e);
    EXPECT_EQ(s, "@"); // has_xattr = true
}
