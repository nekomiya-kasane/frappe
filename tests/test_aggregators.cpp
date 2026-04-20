#include "frappe/aggregators.hpp"

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
        make_entry("a.cpp", 100), make_entry("b.cpp", 200), make_entry("c.hpp", 300),
        make_entry("d.hpp", 400), make_entry("e.txt", 500),
    };
}

} // namespace

// ── First / Last ────────────────────────────────────────────────────────

TEST(AggregatorsTest, First) {
    auto entries = sample();
    auto result = entries | entry::first;
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "a.cpp");
}

TEST(AggregatorsTest, FirstEmpty) {
    std::vector<file_entry> entries;
    auto result = entries | entry::first;
    EXPECT_FALSE(result.has_value());
}

TEST(AggregatorsTest, Last) {
    auto entries = sample();
    auto result = entries | last;
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "e.txt");
}

// ── Take / Skip ─────────────────────────────────────────────────────────

TEST(AggregatorsTest, Take) {
    auto entries = sample();
    auto result = entries | entry::take(2);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0].name, "a.cpp");
    EXPECT_EQ(result[1].name, "b.cpp");
}

TEST(AggregatorsTest, TakeMoreThanSize) {
    auto entries = sample();
    auto result = entries | entry::take(100);
    EXPECT_EQ(result.size(), 5u);
}

TEST(AggregatorsTest, Skip) {
    auto entries = sample();
    auto result = entries | skip(3);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0].name, "d.hpp");
    EXPECT_EQ(result[1].name, "e.txt");
}

TEST(AggregatorsTest, SkipAll) {
    auto entries = sample();
    auto result = entries | skip(100);
    EXPECT_TRUE(result.empty());
}

// ── Count ───────────────────────────────────────────────────────────────

TEST(AggregatorsTest, Count) {
    auto entries = sample();
    auto n = entries | count;
    EXPECT_EQ(n, 5u);
}

TEST(AggregatorsTest, CountIf) {
    auto entries = sample();
    auto n = entries | count_if([](const file_entry &e) { return e.size > 200; });
    EXPECT_EQ(n, 3u);
}

// ── Total / Avg size ────────────────────────────────────────────────────

TEST(AggregatorsTest, TotalSize) {
    auto entries = sample();
    auto total = entries | entry::total_size;
    EXPECT_EQ(total, 1500u);
}

TEST(AggregatorsTest, AvgSize) {
    auto entries = sample();
    auto avg = entries | avg_size;
    EXPECT_DOUBLE_EQ(avg, 300.0);
}

// ── Min / Max ───────────────────────────────────────────────────────────

TEST(AggregatorsTest, Smallest) {
    auto entries = sample();
    auto result = entries | smallest;
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "a.cpp");
    EXPECT_EQ(result->size, 100u);
}

TEST(AggregatorsTest, Largest) {
    auto entries = sample();
    auto result = entries | largest;
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "e.txt");
    EXPECT_EQ(result->size, 500u);
}

TEST(AggregatorsTest, MinByEmpty) {
    std::vector<file_entry> entries;
    auto result = entries | smallest;
    EXPECT_FALSE(result.has_value());
}

// ── Group by ────────────────────────────────────────────────────────────

TEST(AggregatorsTest, GroupByExtension) {
    auto entries = sample();
    auto groups = entries | group_by(proj::extension);
    EXPECT_EQ(groups.size(), 3u);
    EXPECT_EQ(groups[".cpp"].size(), 2u);
    EXPECT_EQ(groups[".hpp"].size(), 2u);
    EXPECT_EQ(groups[".txt"].size(), 1u);
}

// ── Count by ────────────────────────────────────────────────────────────

TEST(AggregatorsTest, CountByExtension) {
    auto entries = sample();
    auto counts = entries | count_by(proj::extension);
    EXPECT_EQ(counts[".cpp"], 2u);
    EXPECT_EQ(counts[".hpp"], 2u);
    EXPECT_EQ(counts[".txt"], 1u);
}

// ── Sum by ──────────────────────────────────────────────────────────────

TEST(AggregatorsTest, SumByExtension) {
    auto entries = sample();
    auto sums = entries | sum_by(proj::extension, proj::size);
    EXPECT_EQ(sums[".cpp"], 300u); // 100+200
    EXPECT_EQ(sums[".hpp"], 700u); // 300+400
    EXPECT_EQ(sums[".txt"], 500u);
}

// ── Partition ───────────────────────────────────────────────────────────

TEST(AggregatorsTest, PartitionBy) {
    auto entries = sample();
    auto [big, small_] = entries | partition_by([](const file_entry &e) { return e.size > 250; });
    EXPECT_EQ(big.size(), 3u);
    EXPECT_EQ(small_.size(), 2u);
}

// ── Chunk ───────────────────────────────────────────────────────────────

TEST(AggregatorsTest, Chunk) {
    auto entries = sample();
    auto chunks = entries | chunk(2);
    ASSERT_EQ(chunks.size(), 3u);
    EXPECT_EQ(chunks[0].size(), 2u);
    EXPECT_EQ(chunks[1].size(), 2u);
    EXPECT_EQ(chunks[2].size(), 1u);
}

// ── Enumerate ───────────────────────────────────────────────────────────

TEST(AggregatorsTest, Enumerate) {
    auto entries = sample();
    auto result = entries | enumerate;
    ASSERT_EQ(result.size(), 5u);
    EXPECT_EQ(result[0].first, 0u);
    EXPECT_EQ(result[0].second.name, "a.cpp");
    EXPECT_EQ(result[4].first, 4u);
}

TEST(AggregatorsTest, EnumerateFrom) {
    auto entries = sample();
    auto result = entries | enumerate_from(10);
    EXPECT_EQ(result[0].first, 10u);
    EXPECT_EQ(result[4].first, 14u);
}

// ── Nth ─────────────────────────────────────────────────────────────────

TEST(AggregatorsTest, Nth) {
    auto entries = sample();
    auto result = entries | nth(2);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "c.hpp");
}

TEST(AggregatorsTest, NthOutOfRange) {
    auto entries = sample();
    auto result = entries | nth(100);
    EXPECT_FALSE(result.has_value());
}

// ── Any / All / None ────────────────────────────────────────────────────

TEST(AggregatorsTest, AnyOf) {
    auto entries = sample();
    EXPECT_TRUE(entries | any_of([](const file_entry &e) { return e.size == 500; }));
    EXPECT_FALSE(entries | any_of([](const file_entry &e) { return e.size == 999; }));
}

TEST(AggregatorsTest, AllOf) {
    auto entries = sample();
    EXPECT_TRUE(entries | all_of([](const file_entry &e) { return e.size > 0; }));
    EXPECT_FALSE(entries | all_of([](const file_entry &e) { return e.size > 200; }));
}

TEST(AggregatorsTest, NoneOf) {
    auto entries = sample();
    EXPECT_TRUE(entries | none_of([](const file_entry &e) { return e.size > 1000; }));
    EXPECT_FALSE(entries | none_of([](const file_entry &e) { return e.size > 400; }));
}

// ── Dedup ───────────────────────────────────────────────────────────────

TEST(AggregatorsTest, DedupByExtension) {
    auto entries = sample();
    auto result = entries | dedup_by(proj::extension);
    // .cpp, .hpp, .txt — 3 unique extensions, keeps first of each
    EXPECT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].name, "a.cpp");
    EXPECT_EQ(result[1].name, "c.hpp");
    EXPECT_EQ(result[2].name, "e.txt");
}

// ── TakeWhile / DropWhile ───────────────────────────────────────────────

TEST(AggregatorsTest, TakeWhile) {
    auto entries = sample();
    auto result = entries | take_while([](const file_entry &e) { return e.size < 300; });
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0].name, "a.cpp");
    EXPECT_EQ(result[1].name, "b.cpp");
}

TEST(AggregatorsTest, DropWhile) {
    auto entries = sample();
    auto result = entries | drop_while([](const file_entry &e) { return e.size < 300; });
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].name, "c.hpp");
}

// ── Collect ─────────────────────────────────────────────────────────────

TEST(AggregatorsTest, Collect) {
    auto entries = sample();
    auto view = entries | std::views::filter([](const file_entry &e) { return e.size > 200; });
    auto result = view | collect;
    EXPECT_EQ(result.size(), 3u);
}
