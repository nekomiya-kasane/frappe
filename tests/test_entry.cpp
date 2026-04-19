#include <gtest/gtest.h>
#include "frappe/entry.hpp"
#include <filesystem>
#include <fstream>
#include <set>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace frappe;

class EntryTest : public ::testing::Test {
protected:
    std::filesystem::path tmp_dir_;

    void SetUp() override {
        tmp_dir_ = std::filesystem::temp_directory_path() / "frappe_entry_test";
        std::filesystem::remove_all(tmp_dir_);
        std::filesystem::create_directories(tmp_dir_);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(tmp_dir_, ec);
    }

    void touch(const std::filesystem::path& p, const std::string& content = "") {
        std::ofstream ofs(p);
        if (!content.empty()) ofs << content;
    }
};

// ── get_file_entry(path) ────────────────────────────────────────────────

TEST_F(EntryTest, GetFileEntryRegularFile) {
    auto f = tmp_dir_ / "hello.txt";
    touch(f, "hello world");

    auto result = get_file_entry(f);
    ASSERT_TRUE(result.has_value()) << "get_file_entry failed";
    auto& e = result.value();

    EXPECT_EQ(e.name, "hello.txt");
    EXPECT_EQ(e.stem, "hello");
    EXPECT_EQ(e.extension, ".txt");
    EXPECT_EQ(e.type, file_type::regular);
    EXPECT_GT(e.size, 0u);
    EXPECT_FALSE(e.is_symlink);
}

TEST_F(EntryTest, GetFileEntryDirectory) {
    auto d = tmp_dir_ / "subdir";
    std::filesystem::create_directory(d);

    auto result = get_file_entry(d);
    ASSERT_TRUE(result.has_value());
    auto& e = result.value();

    EXPECT_EQ(e.name, "subdir");
    EXPECT_EQ(e.type, file_type::directory);
}

TEST_F(EntryTest, GetFileEntryNonExistent) {
    auto result = get_file_entry(tmp_dir_ / "does_not_exist.txt");
    EXPECT_FALSE(result.has_value());
}

TEST_F(EntryTest, GetFileEntryEmptyFile) {
    auto f = tmp_dir_ / "empty.dat";
    touch(f);

    auto result = get_file_entry(f);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size, 0u);
    EXPECT_EQ(result->type, file_type::regular);
}

TEST_F(EntryTest, GetFileEntryPathPreserved) {
    auto f = tmp_dir_ / "path_test.bin";
    touch(f, "data");

    auto result = get_file_entry(f);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::filesystem::canonical(result->file_path),
              std::filesystem::canonical(f));
}

// ── get_file_entry(directory_entry) ─────────────────────────────────────

TEST_F(EntryTest, GetFileEntryFromDirEntry) {
    auto f = tmp_dir_ / "from_dirent.txt";
    touch(f, "content");

    std::filesystem::directory_entry de(f);
    auto e = get_file_entry(de);

    EXPECT_EQ(e.name, "from_dirent.txt");
    EXPECT_EQ(e.type, file_type::regular);
    EXPECT_GT(e.size, 0u);
}

// ── list_entries ────────────────────────────────────────────────────────

TEST_F(EntryTest, ListEntriesFlat) {
    touch(tmp_dir_ / "a.txt");
    touch(tmp_dir_ / "b.txt");
    touch(tmp_dir_ / "c.txt");

    auto entries = list_entries(tmp_dir_);
    EXPECT_EQ(entries.size(), 3u);

    std::set<std::string> names;
    for (auto& e : entries) names.insert(e.name);
    EXPECT_TRUE(names.count("a.txt"));
    EXPECT_TRUE(names.count("b.txt"));
    EXPECT_TRUE(names.count("c.txt"));
}

TEST_F(EntryTest, ListEntriesEmpty) {
    auto entries = list_entries(tmp_dir_);
    EXPECT_TRUE(entries.empty());
}

TEST_F(EntryTest, ListEntriesMixed) {
    touch(tmp_dir_ / "file.txt");
    std::filesystem::create_directory(tmp_dir_ / "subdir");

    auto entries = list_entries(tmp_dir_);
    EXPECT_EQ(entries.size(), 2u);
}

TEST_F(EntryTest, ListEntriesWithOptions) {
    touch(tmp_dir_ / "visible.txt");

    list_options opts;
    opts.include_hidden = false;
    auto entries = list_entries(tmp_dir_, opts);
    EXPECT_GE(entries.size(), 1u);
}

// ── list_entries_recursive ──────────────────────────────────────────────

TEST_F(EntryTest, ListEntriesRecursive) {
    touch(tmp_dir_ / "top.txt");
    auto sub = tmp_dir_ / "sub";
    std::filesystem::create_directory(sub);
    touch(sub / "nested.txt");

    auto entries = list_entries_recursive(tmp_dir_);
    // Should find: top.txt, sub/, sub/nested.txt
    EXPECT_GE(entries.size(), 3u);

    std::set<std::string> names;
    for (auto& e : entries) names.insert(e.name);
    EXPECT_TRUE(names.count("top.txt"));
    EXPECT_TRUE(names.count("nested.txt"));
    EXPECT_TRUE(names.count("sub"));
}

// ── is_hidden_file ──────────────────────────────────────────────────────

TEST_F(EntryTest, IsHiddenFileHidden) {
    auto f = tmp_dir_ / "hidden_file";
    touch(f);
#ifdef _WIN32
    SetFileAttributesW(f.c_str(), FILE_ATTRIBUTE_HIDDEN);
#else
    // On POSIX, rename to dot-prefix
    auto dot_f = tmp_dir_ / ".hidden_file";
    std::filesystem::rename(f, dot_f);
    f = dot_f;
#endif
    EXPECT_TRUE(is_hidden_file(f));
}

TEST_F(EntryTest, IsHiddenFileNormal) {
    auto f = tmp_dir_ / "visible.txt";
    touch(f);
    EXPECT_FALSE(is_hidden_file(f));
}

// ── file_type_indicator ─────────────────────────────────────────────────

TEST_F(EntryTest, FileTypeIndicatorRegular) {
    EXPECT_EQ(file_type_indicator(file_type::regular), ' ');
}

TEST_F(EntryTest, FileTypeIndicatorDirectory) {
    EXPECT_EQ(file_type_indicator(file_type::directory), '/');
}

TEST_F(EntryTest, FileTypeIndicatorSymlink) {
    EXPECT_EQ(file_type_indicator(file_type::symlink), '@');
}

TEST_F(EntryTest, FileTypeIndicatorFromEntry) {
    auto f = tmp_dir_ / "indicator.txt";
    touch(f);
    auto result = get_file_entry(f);
    ASSERT_TRUE(result.has_value());
    char ind = file_type_indicator(*result);
    // Regular file: ' ' (no exec) or '*' (has exec perms, common on Windows)
    EXPECT_TRUE(ind == ' ' || ind == '*') << "got: '" << ind << "'";
}

// ── Comparison operators ────────────────────────────────────────────────

TEST_F(EntryTest, ComparisonOperators) {
    auto fa = tmp_dir_ / "aaa.txt";
    auto fb = tmp_dir_ / "bbb.txt";
    touch(fa);
    touch(fb);

    auto ra = get_file_entry(fa);
    auto rb = get_file_entry(fb);
    ASSERT_TRUE(ra.has_value() && rb.has_value());

    EXPECT_TRUE(*ra < *rb);
    EXPECT_FALSE(*ra == *rb);
    EXPECT_TRUE(*ra == *ra);
}
