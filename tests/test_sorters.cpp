#include "frappe/sorters.hpp"

#include <gtest/gtest.h>
#include <vector>

using namespace frappe;

namespace {

file_entry make_entry(const std::string &name, std::uintmax_t size = 0) {
    file_entry e;
    e.file_path = name;
    e.name = name;
    e.stem = std::filesystem::path(name).stem().string();
    e.extension = std::filesystem::path(name).extension().string();
    e.type = file_type::regular;
    e.size = size;
    e.hard_link_count = 1;
    return e;
}

std::vector<file_entry> sample() {
    return {
        make_entry("charlie.txt", 300),
        make_entry("alpha.txt", 100),
        make_entry("bravo.txt", 200),
    };
}

} // namespace

// ── Sort by name ────────────────────────────────────────────────────────

TEST(SortersTest, SortByName) {
    auto entries = sample();
    auto result = entries | entry::sort_by_name;
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].name, "alpha.txt");
    EXPECT_EQ(result[1].name, "bravo.txt");
    EXPECT_EQ(result[2].name, "charlie.txt");
}

TEST(SortersTest, SortByNameDesc) {
    auto entries = sample();
    auto result = entries | entry::sort_by_name_desc;
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].name, "charlie.txt");
    EXPECT_EQ(result[2].name, "alpha.txt");
}

// ── Sort by size ────────────────────────────────────────────────────────

TEST(SortersTest, SortBySize) {
    auto entries = sample();
    auto result = entries | entry::sort_by_size;
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].size, 100u);
    EXPECT_EQ(result[1].size, 200u);
    EXPECT_EQ(result[2].size, 300u);
}

TEST(SortersTest, SortBySizeDesc) {
    auto entries = sample();
    auto result = entries | entry::sort_by_size_desc;
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].size, 300u);
    EXPECT_EQ(result[2].size, 100u);
}

// ── Sort by extension ───────────────────────────────────────────────────

TEST(SortersTest, SortByExtension) {
    std::vector<file_entry> entries = {
        make_entry("file.cpp", 10),
        make_entry("file.hpp", 20),
        make_entry("file.c", 30),
    };
    auto result = entries | entry::sort_by_extension;
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].extension, ".c");
    EXPECT_EQ(result[1].extension, ".cpp");
    EXPECT_EQ(result[2].extension, ".hpp");
}

// ── Reverse ─────────────────────────────────────────────────────────────

TEST(SortersTest, Reverse) {
    auto entries = sample();
    auto result = entries | entry::reverse;
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].name, "bravo.txt");
    EXPECT_EQ(result[2].name, "charlie.txt");
}

// ── Natural sort ────────────────────────────────────────────────────────

TEST(SortersTest, NaturalSort) {
    std::vector<file_entry> entries = {
        make_entry("file10.txt"),
        make_entry("file2.txt"),
        make_entry("file1.txt"),
        make_entry("file20.txt"),
    };
    auto result = entries | sort_natural;
    ASSERT_EQ(result.size(), 4u);
    EXPECT_EQ(result[0].name, "file1.txt");
    EXPECT_EQ(result[1].name, "file2.txt");
    EXPECT_EQ(result[2].name, "file10.txt");
    EXPECT_EQ(result[3].name, "file20.txt");
}

TEST(SortersTest, NaturalSortDesc) {
    std::vector<file_entry> entries = {
        make_entry("file10.txt"),
        make_entry("file2.txt"),
        make_entry("file1.txt"),
    };
    auto result = entries | sort_natural_desc;
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].name, "file10.txt");
    EXPECT_EQ(result[1].name, "file2.txt");
    EXPECT_EQ(result[2].name, "file1.txt");
}

// ── Desc/Asc chaining ──────────────────────────────────────────────────

TEST(SortersTest, DescChain) {
    auto entries = sample();
    auto result = entries | entry::sort_by_size.desc();
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].size, 300u);
}

TEST(SortersTest, AscChain) {
    auto entries = sample();
    auto result = entries | entry::sort_by_size.asc();
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].size, 100u);
}

// ── Empty input ─────────────────────────────────────────────────────────

TEST(SortersTest, EmptyInput) {
    std::vector<file_entry> entries;
    auto result = entries | entry::sort_by_name;
    EXPECT_TRUE(result.empty());
}

// ── Single element ──────────────────────────────────────────────────────

TEST(SortersTest, SingleElement) {
    std::vector<file_entry> entries = {make_entry("only.txt", 42)};
    auto result = entries | entry::sort_by_size;
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].name, "only.txt");
}
