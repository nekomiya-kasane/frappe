#include "frappe/projections.hpp"

#include <gtest/gtest.h>
#include <vector>

using namespace frappe;

namespace {

    file_entry make_entry(const std::string &name, file_type type, std::uintmax_t size = 0, bool hidden = false) {
        file_entry e;
        e.file_path = "/tmp/" + name;
        e.name = name;
        e.stem = std::filesystem::path(name).stem().string();
        e.extension = std::filesystem::path(name).extension().string();
        e.type = type;
        e.size = size;
        e.is_hidden = hidden;
        e.is_symlink = false;
        e.is_broken_link = false;
        e.has_acl = false;
        e.has_xattr = false;
        e.permissions = perms::owner_read | perms::owner_write;
        e.hard_link_count = 1;
        e.inode = 42;
        e.uid = 1000;
        e.gid = 1000;
        e.owner = "user";
        e.group = "staff";
        e.mime_type = "text/plain";
        return e;
    }

} // namespace

// ── Basic attribute projections ─────────────────────────────────────────

TEST(ProjectionsTest, FilePath) {
    auto e = make_entry("test.txt", file_type::regular, 100);
    EXPECT_EQ(proj::file_path(e).string(), "/tmp/test.txt");
}

TEST(ProjectionsTest, Name) {
    auto e = make_entry("test.txt", file_type::regular);
    EXPECT_EQ(proj::name(e), "test.txt");
}

TEST(ProjectionsTest, Stem) {
    auto e = make_entry("test.txt", file_type::regular);
    EXPECT_EQ(proj::stem(e), "test");
}

TEST(ProjectionsTest, Extension) {
    auto e = make_entry("test.txt", file_type::regular);
    EXPECT_EQ(proj::extension(e), ".txt");
}

// ── Type projections ────────────────────────────────────────────────────

TEST(ProjectionsTest, Type) {
    auto e = make_entry("dir", file_type::directory);
    EXPECT_EQ(proj::type(e), file_type::directory);
}

TEST(ProjectionsTest, IsSymlink) {
    auto e = make_entry("link", file_type::regular);
    EXPECT_FALSE(proj::is_symlink(e));
}

TEST(ProjectionsTest, IsHidden) {
    auto e = make_entry(".hidden", file_type::regular, 0, true);
    EXPECT_TRUE(proj::is_hidden(e));
}

// ── Size projections ────────────────────────────────────────────────────

TEST(ProjectionsTest, Size) {
    auto e = make_entry("big.bin", file_type::regular, 999);
    EXPECT_EQ(proj::size(e), 999u);
}

TEST(ProjectionsTest, HardLinkCount) {
    auto e = make_entry("f.txt", file_type::regular);
    EXPECT_EQ(proj::hard_link_count(e), 1u);
}

TEST(ProjectionsTest, Inode) {
    auto e = make_entry("f.txt", file_type::regular);
    EXPECT_EQ(proj::inode(e), 42u);
}

// ── Ownership projections ───────────────────────────────────────────────

TEST(ProjectionsTest, Owner) {
    auto e = make_entry("f.txt", file_type::regular);
    EXPECT_EQ(proj::owner(e), "user");
}

TEST(ProjectionsTest, Group) {
    auto e = make_entry("f.txt", file_type::regular);
    EXPECT_EQ(proj::group(e), "staff");
}

TEST(ProjectionsTest, Uid) {
    auto e = make_entry("f.txt", file_type::regular);
    EXPECT_EQ(proj::uid(e), 1000u);
}

TEST(ProjectionsTest, Gid) {
    auto e = make_entry("f.txt", file_type::regular);
    EXPECT_EQ(proj::gid(e), 1000u);
}

// ── Metadata projections ────────────────────────────────────────────────

TEST(ProjectionsTest, Permissions) {
    auto e = make_entry("f.txt", file_type::regular);
    EXPECT_EQ(proj::permissions(e), perms::owner_read | perms::owner_write);
}

TEST(ProjectionsTest, MimeType) {
    auto e = make_entry("f.txt", file_type::regular);
    EXPECT_EQ(proj::mime_type(e), "text/plain");
}

TEST(ProjectionsTest, HasAcl) {
    auto e = make_entry("f.txt", file_type::regular);
    EXPECT_FALSE(proj::has_acl(e));
}

TEST(ProjectionsTest, HasXattr) {
    auto e = make_entry("f.txt", file_type::regular);
    EXPECT_FALSE(proj::has_xattr(e));
}

// ── Boolean projections ─────────────────────────────────────────────────

TEST(ProjectionsTest, IsFile) {
    auto e = make_entry("f.txt", file_type::regular);
    EXPECT_TRUE(proj::is_file(e));
    EXPECT_TRUE(proj::is_regular(e));
}

TEST(ProjectionsTest, IsDirectory) {
    auto e = make_entry("dir", file_type::directory);
    EXPECT_TRUE(proj::is_directory(e));
    EXPECT_FALSE(proj::is_file(e));
}

TEST(ProjectionsTest, IsEmpty) {
    auto e = make_entry("empty.txt", file_type::regular, 0);
    EXPECT_TRUE(proj::is_empty(e));
    auto e2 = make_entry("full.txt", file_type::regular, 100);
    EXPECT_FALSE(proj::is_empty(e2));
}

// ── Permission check projections ────────────────────────────────────────

TEST(ProjectionsTest, IsReadable) {
    auto e = make_entry("f.txt", file_type::regular);
    EXPECT_TRUE(proj::is_readable(e));
}

TEST(ProjectionsTest, IsWritable) {
    auto e = make_entry("f.txt", file_type::regular);
    EXPECT_TRUE(proj::is_writable(e));
}

TEST(ProjectionsTest, IsExecutable) {
    auto e = make_entry("f.txt", file_type::regular);
    EXPECT_FALSE(proj::is_executable(e));
}

// ── Derived projections ─────────────────────────────────────────────────

TEST(ProjectionsTest, ParentPath) {
    auto e = make_entry("f.txt", file_type::regular);
    EXPECT_EQ(proj::parent_path(e).string(), "/tmp");
}

// ── Usage with ranges ───────────────────────────────────────────────────

TEST(ProjectionsTest, UsedInSort) {
    std::vector<file_entry> entries = {
        make_entry("c.txt", file_type::regular, 300),
        make_entry("a.txt", file_type::regular, 100),
        make_entry("b.txt", file_type::regular, 200),
    };
    std::ranges::sort(entries, {}, proj::name);
    EXPECT_EQ(entries[0].name, "a.txt");
    EXPECT_EQ(entries[1].name, "b.txt");
    EXPECT_EQ(entries[2].name, "c.txt");
}

TEST(ProjectionsTest, UsedInSortBySize) {
    std::vector<file_entry> entries = {
        make_entry("c.txt", file_type::regular, 300),
        make_entry("a.txt", file_type::regular, 100),
        make_entry("b.txt", file_type::regular, 200),
    };
    std::ranges::sort(entries, {}, proj::size);
    EXPECT_EQ(entries[0].size, 100u);
    EXPECT_EQ(entries[1].size, 200u);
    EXPECT_EQ(entries[2].size, 300u);
}
