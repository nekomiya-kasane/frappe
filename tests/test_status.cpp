#include <gtest/gtest.h>
#include "frappe/status.hpp"
#include <filesystem>
#include <fstream>

class StatusTest : public ::testing::Test {
protected:
    void SetUp() override {
        _test_dir = std::filesystem::temp_directory_path() / "frappe_status_test";
        std::filesystem::create_directories(_test_dir);
        
        _test_file = _test_dir / "test_file.txt";
        std::ofstream(_test_file) << "test content";
        
        _test_subdir = _test_dir / "subdir";
        std::filesystem::create_directories(_test_subdir);
        
        _empty_file = _test_dir / "empty.txt";
        std::ofstream ofs(_empty_file);
        ofs.close();
    }
    
    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(_test_dir, ec);
    }
    
    std::filesystem::path _test_dir;
    std::filesystem::path _test_file;
    std::filesystem::path _test_subdir;
    std::filesystem::path _empty_file;
};

TEST_F(StatusTest, Status) {
    auto st = frappe::status(_test_file);
    EXPECT_TRUE(st.has_value());
    EXPECT_EQ(st->type(), frappe::file_type::regular);
}

TEST_F(StatusTest, Exists) {
    auto r1 = frappe::exists(_test_file);
    EXPECT_TRUE(r1.has_value());
    EXPECT_TRUE(*r1);
    
    auto r2 = frappe::exists(_test_dir / "nonexistent");
    EXPECT_TRUE(r2.has_value());
    EXPECT_FALSE(*r2);
}

TEST_F(StatusTest, IsRegularFile) {
    auto r1 = frappe::is_regular_file(_test_file);
    EXPECT_TRUE(r1.has_value() && *r1);
    
    auto r2 = frappe::is_regular_file(_test_subdir);
    EXPECT_TRUE(r2.has_value() && !*r2);
}

TEST_F(StatusTest, IsDirectory) {
    auto r1 = frappe::is_directory(_test_subdir);
    EXPECT_TRUE(r1.has_value() && *r1);
    
    auto r2 = frappe::is_directory(_test_file);
    EXPECT_TRUE(r2.has_value() && !*r2);
}

TEST_F(StatusTest, IsEmpty) {
    auto r1 = frappe::is_empty(_empty_file);
    EXPECT_TRUE(r1.has_value() && *r1);
    
    auto r2 = frappe::is_empty(_test_file);
    EXPECT_TRUE(r2.has_value() && !*r2);
    
    auto empty_dir = _test_dir / "empty_dir";
    std::filesystem::create_directories(empty_dir);
    auto r3 = frappe::is_empty(empty_dir);
    EXPECT_TRUE(r3.has_value() && *r3);
}

TEST_F(StatusTest, FileSize) {
    auto size = frappe::file_size(_test_file);
    EXPECT_TRUE(size.has_value());
    EXPECT_GT(*size, 0u);
    
    auto empty_size = frappe::file_size(_empty_file);
    EXPECT_TRUE(empty_size.has_value());
    EXPECT_EQ(*empty_size, 0u);
}

TEST_F(StatusTest, LastWriteTime) {
    auto time = frappe::last_write_time(_test_file);
    EXPECT_TRUE(time.has_value());
    EXPECT_NE(*time, frappe::file_time_type::min());
}

TEST_F(StatusTest, CreationTime) {
    auto time = frappe::creation_time(_test_file);
    EXPECT_TRUE(time.has_value());
    EXPECT_NE(*time, frappe::file_time_type::min());
}

TEST_F(StatusTest, LastAccessTime) {
    auto time = frappe::last_access_time(_test_file);
    EXPECT_TRUE(time.has_value());
    EXPECT_NE(*time, frappe::file_time_type::min());
}

TEST_F(StatusTest, HardLinkCount) {
    auto count = frappe::hard_link_count(_test_file);
    EXPECT_TRUE(count.has_value());
    EXPECT_GE(*count, 1u);
}

TEST_F(StatusTest, IsHardLink) {
    auto r = frappe::is_hard_link(_test_file);
    EXPECT_TRUE(r.has_value());
    EXPECT_FALSE(*r);
}

TEST_F(StatusTest, FileId) {
    auto id = frappe::get_file_id(_test_file);
    EXPECT_TRUE(id.has_value());
    EXPECT_NE(id->inode, 0u);
}

TEST_F(StatusTest, FileIdEquality) {
    auto id1 = frappe::get_file_id(_test_file);
    auto id2 = frappe::get_file_id(_test_file);
    EXPECT_TRUE(id1.has_value() && id2.has_value());
    EXPECT_EQ(*id1, *id2);
    
    auto id3 = frappe::get_file_id(_empty_file);
    EXPECT_TRUE(id3.has_value());
    EXPECT_NE(*id1, *id3);
}

TEST_F(StatusTest, Equivalent) {
    auto r1 = frappe::equivalent(_test_file, _test_file);
    EXPECT_TRUE(r1.has_value() && *r1);
    
    auto r2 = frappe::equivalent(_test_file, _empty_file);
    EXPECT_TRUE(r2.has_value() && !*r2);
}

TEST_F(StatusTest, FileTypeName) {
    EXPECT_EQ(frappe::file_type_name(frappe::file_type::regular), "regular");
    EXPECT_EQ(frappe::file_type_name(frappe::file_type::directory), "directory");
    EXPECT_EQ(frappe::file_type_name(frappe::file_type::symlink), "symlink");
}

TEST_F(StatusTest, LinkInfo) {
    auto info = frappe::get_link_info(_test_file);
    EXPECT_TRUE(info.has_value());
    EXPECT_EQ(info->type, frappe::link_type::none);
}

TEST_F(StatusTest, MaxHardLinks) {
    auto max = frappe::max_hard_links(_test_dir);
    EXPECT_TRUE(max.has_value());
    EXPECT_GT(*max, 0u);
}

// Link creation tests
TEST_F(StatusTest, CreateSymlink) {
    auto link_path = _test_dir / "test_symlink";
    auto result = frappe::create_symlink(_test_file, link_path);
    EXPECT_TRUE(result.has_value());
    
    auto is_link = frappe::is_symlink(link_path);
    EXPECT_TRUE(is_link.has_value() && *is_link);
    
    auto target = frappe::read_symlink(link_path);
    EXPECT_TRUE(target.has_value());
    
    std::filesystem::remove(link_path);
}

TEST_F(StatusTest, CreateDirectorySymlink) {
    auto link_path = _test_dir / "test_dir_symlink";
    auto result = frappe::create_directory_symlink(_test_dir, link_path);
    EXPECT_TRUE(result.has_value());
    
    auto is_link = frappe::is_symlink(link_path);
    EXPECT_TRUE(is_link.has_value() && *is_link);
    
    std::filesystem::remove(link_path);
}

TEST_F(StatusTest, CreateHardLink) {
    auto link_path = _test_dir / "test_hardlink";
    auto result = frappe::create_hard_link(_test_file, link_path);
    EXPECT_TRUE(result.has_value());
    
    auto are_same = frappe::are_hard_links(_test_file, link_path);
    EXPECT_TRUE(are_same.has_value() && *are_same);
    
    auto count = frappe::hard_link_count(_test_file);
    EXPECT_TRUE(count.has_value());
    EXPECT_GE(*count, 2u);
    
    std::filesystem::remove(link_path);
}
