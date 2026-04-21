#include "frappe/traversal.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <set>

namespace fs = std::filesystem;

class TraversalTest : public ::testing::Test {
  protected:
    fs::path tmp_dir_;

    void SetUp() override {
        tmp_dir_ = fs::temp_directory_path() / "frappe_traversal_test";
        std::error_code ec;
        fs::remove_all(tmp_dir_, ec);
        fs::create_directories(tmp_dir_);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(tmp_dir_, ec);
    }

    void touch(const fs::path &p) {
        fs::create_directories(p.parent_path());
        std::ofstream f(p);
        f << "test";
    }
};

TEST_F(TraversalTest, TraverseCollectEmpty) {
    auto entries = frappe::traverse_collect(tmp_dir_);
    EXPECT_TRUE(entries.empty());
}

TEST_F(TraversalTest, TraverseCollectFlat) {
    touch(tmp_dir_ / "a.txt");
    touch(tmp_dir_ / "b.txt");
    touch(tmp_dir_ / "c.txt");

    auto entries = frappe::traverse_collect(tmp_dir_);
    EXPECT_EQ(entries.size(), 3u);

    std::set<std::string> names;
    for (auto &e : entries) {
        names.insert(e.name);
    }
    EXPECT_TRUE(names.count("a.txt"));
    EXPECT_TRUE(names.count("b.txt"));
    EXPECT_TRUE(names.count("c.txt"));
}

TEST_F(TraversalTest, TraverseCollectNested) {
    touch(tmp_dir_ / "file.txt");
    touch(tmp_dir_ / "sub" / "nested.txt");
    touch(tmp_dir_ / "sub" / "deep" / "deep.txt");

    auto entries = frappe::traverse_collect(tmp_dir_);
    // Should find: file.txt, sub/, sub/nested.txt, sub/deep/, sub/deep/deep.txt
    EXPECT_GE(entries.size(), 3u); // At least the 3 files
}

TEST_F(TraversalTest, TraverseWithMaxDepth) {
    touch(tmp_dir_ / "top.txt");
    touch(tmp_dir_ / "d1" / "mid.txt");
    touch(tmp_dir_ / "d1" / "d2" / "bottom.txt");

    frappe::traversal_options opts;
    opts.max_depth = 1;
    auto entries = frappe::traverse_collect(tmp_dir_, opts);

    // Should find top.txt and d1/ (and maybe d1/mid.txt at depth 1)
    // but NOT d1/d2/bottom.txt
    std::set<std::string> names;
    for (auto &e : entries) {
        names.insert(e.name);
    }
    EXPECT_TRUE(names.count("top.txt"));
    EXPECT_FALSE(names.count("bottom.txt"));
}

TEST_F(TraversalTest, TraverseWithMaxEntries) {
    for (int i = 0; i < 20; ++i) {
        touch(tmp_dir_ / ("file" + std::to_string(i) + ".txt"));
    }

    frappe::traversal_options opts;
    opts.max_entries = 5;
    auto entries = frappe::traverse_collect(tmp_dir_, opts);
    EXPECT_LE(entries.size(), 5u);
}

TEST_F(TraversalTest, TraverseWithCallback) {
    touch(tmp_dir_ / "a.txt");
    touch(tmp_dir_ / "b.txt");
    touch(tmp_dir_ / "c.txt");

    std::vector<std::string> collected;
    frappe::traverse_with(tmp_dir_, [&](const frappe::file_entry &e) -> bool {
        collected.push_back(e.name);
        return true; // continue
    });
    EXPECT_EQ(collected.size(), 3u);
}

TEST_F(TraversalTest, TraverseWithCallbackEarlyStop) {
    for (int i = 0; i < 10; ++i) {
        touch(tmp_dir_ / ("file" + std::to_string(i) + ".txt"));
    }

    int count = 0;
    frappe::traverse_with(tmp_dir_, [&](const frappe::file_entry &) -> bool {
        return ++count < 3; // stop after 3
    });
    EXPECT_EQ(count, 3);
}

TEST_F(TraversalTest, DirectoryTraverserBasic) {
    touch(tmp_dir_ / "x.txt");
    touch(tmp_dir_ / "y.txt");

    frappe::directory_traverser traverser(tmp_dir_);
    int count = 0;
    for (auto it = traverser.begin(); it != traverser.end(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(TraversalTest, DirectoryTraverserStats) {
    touch(tmp_dir_ / "f1.txt");
    touch(tmp_dir_ / "f2.txt");
    fs::create_directory(tmp_dir_ / "subdir");
    touch(tmp_dir_ / "subdir" / "f3.txt");

    frappe::traversal_options opts;
    opts.include_hidden = true;
    frappe::directory_traverser traverser(tmp_dir_, opts);
    for (auto it = traverser.begin(); it != traverser.end(); ++it) {}

    auto &stats = traverser.stats();
    // 3 files + 1 directory = 4 entries total
    EXPECT_GE(stats.files_visited + stats.dirs_visited, 4u);
    EXPECT_GE(stats.dirs_visited, 1u);
}

TEST_F(TraversalTest, DirectoryTraverserCancel) {
    for (int i = 0; i < 20; ++i) {
        touch(tmp_dir_ / ("file" + std::to_string(i) + ".txt"));
    }

    frappe::directory_traverser traverser(tmp_dir_);
    int count = 0;
    for (auto it = traverser.begin(); it != traverser.end(); ++it) {
        if (++count >= 5) {
            traverser.cancel();
            break;
        }
    }
    EXPECT_TRUE(traverser.is_cancelled());
    EXPECT_LE(count, 6);
}

TEST_F(TraversalTest, SafeRecursiveIterator) {
    touch(tmp_dir_ / "a.txt");
    touch(tmp_dir_ / "sub" / "b.txt");

    int count = 0;
    for (auto &entry : frappe::safe_recursive_directory(tmp_dir_)) {
        (void)entry;
        ++count;
    }
    EXPECT_GE(count, 2); // at least 2 files (may include dirs)
}

TEST_F(TraversalTest, TraverseNonExistentDir) {
    auto entries = frappe::traverse_collect(tmp_dir_ / "nonexistent");
    EXPECT_TRUE(entries.empty());
}
