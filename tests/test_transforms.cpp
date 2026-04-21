#include "frappe/transforms.hpp"

#include <gtest/gtest.h>
#include <map>
#include <set>
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
            make_entry("a.cpp", 100),
            make_entry("b.hpp", 200),
            make_entry("c.txt", 300),
        };
    }

} // namespace

// ── MapTo ───────────────────────────────────────────────────────────────

TEST(TransformsTest, MapToNames) {
    auto entries = sample();
    auto result = entries | names_only;
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], "a.cpp");
    EXPECT_EQ(result[1], "b.hpp");
    EXPECT_EQ(result[2], "c.txt");
}

TEST(TransformsTest, MapToSizes) {
    auto entries = sample();
    auto result = entries | sizes_only;
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], 100u);
    EXPECT_EQ(result[1], 200u);
    EXPECT_EQ(result[2], 300u);
}

TEST(TransformsTest, MapToPaths) {
    auto entries = sample();
    auto result = entries | paths_only;
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].string(), "a.cpp");
}

TEST(TransformsTest, CustomMapTo) {
    auto entries = sample();
    auto result = entries | map_to([](const file_entry &e) { return e.name + "!"; });
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], "a.cpp!");
}

// ── Select (multi-projection) ───────────────────────────────────────────

TEST(TransformsTest, Select) {
    auto entries = sample();
    auto result = entries | select(proj::name, proj::size);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(std::get<0>(result[0]), "a.cpp");
    EXPECT_EQ(std::get<1>(result[0]), 100u);
    EXPECT_EQ(std::get<0>(result[2]), "c.txt");
    EXPECT_EQ(std::get<1>(result[2]), 300u);
}

// ── ToVector ────────────────────────────────────────────────────────────

TEST(TransformsTest, ToVector) {
    auto entries = sample();
    auto view = entries | std::views::filter([](const file_entry &e) { return e.size > 100; });
    auto result = view | to_vector;
    EXPECT_EQ(result.size(), 2u);
}

// ── ToSet ───────────────────────────────────────────────────────────────

TEST(TransformsTest, ToSetByExtension) {
    auto entries = sample();
    auto result = entries | to_set_by(proj::extension);
    EXPECT_EQ(result.size(), 3u);
    EXPECT_TRUE(result.count(".cpp"));
    EXPECT_TRUE(result.count(".hpp"));
    EXPECT_TRUE(result.count(".txt"));
}

// ── ToMap ────────────────────────────────────────────────────────────────

TEST(TransformsTest, NameToPath) {
    auto entries = sample();
    auto result = entries | name_to_path;
    EXPECT_EQ(result.size(), 3u);
    EXPECT_EQ(result["a.cpp"].string(), "a.cpp");
}

TEST(TransformsTest, PathToSize) {
    auto entries = sample();
    auto result = entries | path_to_size;
    EXPECT_EQ(result.size(), 3u);
}

// ── FilterMap ───────────────────────────────────────────────────────────

TEST(TransformsTest, FilterMap) {
    auto entries = sample();
    auto result = entries | filter_map([](const file_entry &e) { return e.size > 100; },
                                       [](const file_entry &e) { return e.name; });
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], "b.hpp");
    EXPECT_EQ(result[1], "c.txt");
}

// ── Tap ─────────────────────────────────────────────────────────────────

TEST(TransformsTest, Tap) {
    auto entries = sample();
    int count = 0;
    auto result = entries | tap([&count](const file_entry &) { ++count; });
    EXPECT_EQ(count, 3);
    EXPECT_EQ(result.size(), 3u);
}

// ── Reduce ──────────────────────────────────────────────────────────────

TEST(TransformsTest, Reduce) {
    auto entries = sample();
    auto total =
        entries | reduce([](std::uintmax_t acc, const file_entry &e) { return acc + e.size; }, std::uintmax_t{0});
    EXPECT_EQ(total, 600u);
}

// ── Scan ────────────────────────────────────────────────────────────────

TEST(TransformsTest, Scan) {
    auto entries = sample();
    auto running =
        entries | scan([](std::uintmax_t acc, const file_entry &e) { return acc + e.size; }, std::uintmax_t{0});
    ASSERT_EQ(running.size(), 3u);
    EXPECT_EQ(running[0], 100u);
    EXPECT_EQ(running[1], 300u);
    EXPECT_EQ(running[2], 600u);
}

// ── Zip ─────────────────────────────────────────────────────────────────

TEST(TransformsTest, ZipWith) {
    auto entries = sample();
    std::vector<int> indices = {1, 2, 3};
    auto result = entries | zip_with(indices);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].second, 1);
    EXPECT_EQ(result[0].first.name, "a.cpp");
}

TEST(TransformsTest, ZipUnequalLength) {
    auto entries = sample();
    std::vector<int> short_vec = {10};
    auto result = entries | zip_with(short_vec);
    EXPECT_EQ(result.size(), 1u);
}

// ── Empty input ─────────────────────────────────────────────────────────

TEST(TransformsTest, EmptyMapTo) {
    std::vector<file_entry> entries;
    auto result = entries | names_only;
    EXPECT_TRUE(result.empty());
}

TEST(TransformsTest, EmptyReduce) {
    std::vector<file_entry> entries;
    auto total =
        entries | reduce([](std::uintmax_t acc, const file_entry &e) { return acc + e.size; }, std::uintmax_t{0});
    EXPECT_EQ(total, 0u);
}
