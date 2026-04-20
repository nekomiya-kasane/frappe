#include "frappe/glob.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

class GlobTest : public ::testing::Test {
  protected:
    void SetUp() override {
        _test_dir = std::filesystem::temp_directory_path() / "frappe_glob_test";
        std::filesystem::create_directories(_test_dir);
        std::filesystem::create_directories(_test_dir / "subdir1");
        std::filesystem::create_directories(_test_dir / "subdir2");
        std::filesystem::create_directories(_test_dir / "subdir1" / "nested");

        create_file(_test_dir / "file1.txt");
        create_file(_test_dir / "file2.txt");
        create_file(_test_dir / "file3.log");
        create_file(_test_dir / "subdir1" / "sub1.txt");
        create_file(_test_dir / "subdir1" / "sub2.cpp");
        create_file(_test_dir / "subdir1" / "nested" / "deep.txt");
        create_file(_test_dir / "subdir2" / "sub3.txt");
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(_test_dir, ec);
    }

    void create_file(const std::filesystem::path &p) { std::ofstream(p).put('x'); }

    std::filesystem::path _test_dir;
};

TEST_F(GlobTest, FnmatchSimple) {
    EXPECT_TRUE(frappe::fnmatch("file.txt", "file.txt"));
    EXPECT_FALSE(frappe::fnmatch("file.txt", "file.log"));
}

TEST_F(GlobTest, FnmatchStar) {
    EXPECT_TRUE(frappe::fnmatch("file.txt", "*.txt"));
    EXPECT_TRUE(frappe::fnmatch("file.txt", "file.*"));
    EXPECT_TRUE(frappe::fnmatch("file.txt", "*"));
    EXPECT_FALSE(frappe::fnmatch("file.txt", "*.log"));
}

TEST_F(GlobTest, FnmatchQuestion) {
    EXPECT_TRUE(frappe::fnmatch("file.txt", "fil?.txt"));
    EXPECT_TRUE(frappe::fnmatch("file.txt", "????.txt"));
    EXPECT_FALSE(frappe::fnmatch("file.txt", "???.txt"));
}

TEST_F(GlobTest, FnmatchBracket) {
    EXPECT_TRUE(frappe::fnmatch("file1.txt", "file[123].txt"));
    EXPECT_TRUE(frappe::fnmatch("file2.txt", "file[1-3].txt"));
    EXPECT_FALSE(frappe::fnmatch("file4.txt", "file[1-3].txt"));
}

TEST_F(GlobTest, FnmatchBracketNegate) {
    EXPECT_FALSE(frappe::fnmatch("file1.txt", "file[!123].txt"));
    EXPECT_TRUE(frappe::fnmatch("file4.txt", "file[!123].txt"));
}

TEST_F(GlobTest, MatchPath) {
    EXPECT_TRUE(frappe::match(_test_dir / "file1.txt", "*.txt"));
    EXPECT_FALSE(frappe::match(_test_dir / "file3.log", "*.txt"));
}

TEST_F(GlobTest, GlobTxtFiles) {
    auto results = frappe::glob(_test_dir, "*.txt");
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(GlobTest, GlobAllFiles) {
    auto results = frappe::glob(_test_dir, "*.*");
    EXPECT_EQ(results.size(), 3u);
}

TEST_F(GlobTest, GlobSubdir) {
    auto results = frappe::glob(_test_dir, "subdir1/*.txt");
    EXPECT_EQ(results.size(), 1u);
}

TEST_F(GlobTest, GlobRecursive) {
    auto results = frappe::glob(_test_dir, "**/*.txt");
    EXPECT_GE(results.size(), 4u);
}

TEST_F(GlobTest, GlobIterator) {
    int count = 0;
    for (const auto &p : frappe::iglob(_test_dir, "*.txt")) {
        EXPECT_TRUE(p.extension() == ".txt");
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(GlobTest, GlobNonExistentDir) {
    auto results = frappe::glob("/nonexistent/path", "*.txt");
    EXPECT_TRUE(results.empty());
}
