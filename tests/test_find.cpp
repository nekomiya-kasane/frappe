#include "frappe/find.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

class FindTest : public ::testing::Test {
  protected:
    void SetUp() override {
        _test_dir = std::filesystem::temp_directory_path() / "frappe_find_test";
        std::filesystem::create_directories(_test_dir);
        std::filesystem::create_directories(_test_dir / "subdir1");
        std::filesystem::create_directories(_test_dir / "subdir2");
        std::filesystem::create_directories(_test_dir / "subdir1" / "nested");
        std::filesystem::create_directories(_test_dir / "empty_dir");

        create_file(_test_dir / "file1.txt", 100);
        create_file(_test_dir / "file2.txt", 200);
        create_file(_test_dir / "file3.log", 50);
        create_file(_test_dir / "FILE4.TXT", 150);
        create_file(_test_dir / "subdir1" / "sub1.txt", 1000);
        create_file(_test_dir / "subdir1" / "sub2.cpp", 500);
        create_file(_test_dir / "subdir1" / "nested" / "deep.txt", 2000);
        create_file(_test_dir / "subdir2" / "sub3.txt", 300);
        create_file(_test_dir / "empty.txt", 0);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(_test_dir, ec);
    }

    void create_file(const std::filesystem::path &p, size_t size) {
        std::ofstream ofs(p, std::ios::binary);
        std::string content(size, 'x');
        ofs.write(content.data(), content.size());
    }

    std::filesystem::path _test_dir;
};

TEST_F(FindTest, FindAll) {
    auto results = frappe::find(_test_dir);
    EXPECT_GT(results.size(), 0u);
}

TEST_F(FindTest, FindByName) {
    auto results = frappe::find(_test_dir, frappe::find_options{}.name("*.txt"));
    EXPECT_GE(results.size(), 4u);
    for (const auto &p : results) {
        EXPECT_EQ(p.extension(), ".txt");
    }
}

TEST_F(FindTest, FindByIname) {
    auto results = frappe::find(_test_dir, frappe::find_options{}.iname("*.txt"));
    EXPECT_GE(results.size(), 5u);
}

TEST_F(FindTest, FindByExtension) {
    auto results = frappe::find(_test_dir, frappe::find_options{}.extension("cpp"));
    EXPECT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].extension(), ".cpp");
}

TEST_F(FindTest, FindByExtensions) {
    auto results = frappe::find(_test_dir, frappe::find_options{}.extensions({"txt", "log"}));
    EXPECT_GE(results.size(), 5u);
}

TEST_F(FindTest, FindFiles) {
    auto results = frappe::find(_test_dir, frappe::find_options{}.is_file());
    for (const auto &p : results) {
        EXPECT_TRUE(std::filesystem::is_regular_file(p));
    }
}

TEST_F(FindTest, FindDirs) {
    auto results = frappe::find(_test_dir, frappe::find_options{}.is_dir());
    for (const auto &p : results) {
        EXPECT_TRUE(std::filesystem::is_directory(p));
    }
}

TEST_F(FindTest, FindBySizeGt) {
    using namespace frappe::literals;
    auto results = frappe::find(_test_dir, frappe::find_options{}.is_file().size_gt(500));
    for (const auto &p : results) {
        EXPECT_GT(std::filesystem::file_size(p), 500u);
    }
}

TEST_F(FindTest, FindBySizeLt) {
    auto results = frappe::find(_test_dir, frappe::find_options{}.is_file().size_lt(200));
    for (const auto &p : results) {
        EXPECT_LT(std::filesystem::file_size(p), 200u);
    }
}

TEST_F(FindTest, FindEmpty) {
    auto results = frappe::find(_test_dir, frappe::find_options{}.empty());
    EXPECT_GE(results.size(), 2u);
}

TEST_F(FindTest, FindMaxDepth) {
    auto results_depth1 = frappe::find(_test_dir, frappe::find_options{}.max_depth(1));
    auto results_depth2 = frappe::find(_test_dir, frappe::find_options{}.max_depth(2));
    EXPECT_LT(results_depth1.size(), results_depth2.size());
}

TEST_F(FindTest, FindByRegex) {
    auto results = frappe::find(_test_dir, frappe::find_options{}.regex("file[0-9]+\\.txt"));
    EXPECT_GE(results.size(), 2u);
}

TEST_F(FindTest, FindLazy) {
    int count = 0;
    for (const auto &p : frappe::find_lazy(_test_dir, frappe::find_options{}.is_file())) {
        EXPECT_TRUE(std::filesystem::is_regular_file(p));
        ++count;
    }
    EXPECT_GT(count, 0);
}

TEST_F(FindTest, FindFirst) {
    auto result = frappe::find_first(_test_dir, frappe::find_options{}.extension("cpp"));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->extension(), ".cpp");
}

TEST_F(FindTest, FindCount) {
    auto count = frappe::find_count(_test_dir, frappe::find_options{}.is_file());
    EXPECT_GT(count, 0u);
}

TEST_F(FindTest, FindWithPredicate) {
    auto results =
        frappe::find(_test_dir, frappe::find_options{}.predicate([](const std::filesystem::directory_entry &e) {
            return e.path().filename().string().starts_with("sub");
        }));
    for (const auto &p : results) {
        EXPECT_TRUE(p.filename().string().starts_with("sub"));
    }
}

TEST_F(FindTest, TotalSize) {
    auto files = frappe::find(_test_dir, frappe::find_options{}.is_file());
    auto total = frappe::total_size(files);
    EXPECT_GT(total, 0u);
}

TEST_F(FindTest, SizeLiterals) {
    using namespace frappe::literals;
    EXPECT_EQ(1_KB, 1024u);
    EXPECT_EQ(1_MB, 1024u * 1024);
    EXPECT_EQ(1_GB, 1024u * 1024 * 1024);
}
