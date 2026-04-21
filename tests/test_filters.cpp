#include "frappe/filters.hpp"

#include <gtest/gtest.h>
#include <ranges>
#include <vector>

using namespace frappe;

namespace {

    file_entry make_entry(const std::string &name, file_type type, std::uintmax_t size = 0, bool hidden = false) {
        file_entry e;
        e.file_path = name;
        e.name = name;
        e.stem = std::filesystem::path(name).stem().string();
        e.extension = std::filesystem::path(name).extension().string();
        e.type = type;
        e.size = size;
        e.is_hidden = hidden;
        e.permissions = perms::owner_read | perms::owner_write;
        e.hard_link_count = 1;
        return e;
    }

    std::vector<file_entry> sample_entries() {
        return {
            make_entry("readme.md", file_type::regular, 1024), make_entry("main.cpp", file_type::regular, 4096),
            make_entry("build", file_type::directory, 0),      make_entry(".gitignore", file_type::regular, 256, true),
            make_entry("lib.hpp", file_type::regular, 2048),   make_entry("empty.txt", file_type::regular, 0),
        };
    }

} // namespace

// ── Type filters ────────────────────────────────────────────────────────

TEST(FiltersTest, FilesOnly) {
    auto entries = sample_entries();
    auto result = entries | files_only;
    // 5 regular files, 1 directory
    EXPECT_EQ(std::ranges::distance(result), 5);
}

TEST(FiltersTest, DirsOnly) {
    auto entries = sample_entries();
    auto result = entries | dirs_only;
    EXPECT_EQ(std::ranges::distance(result), 1);
}

TEST(FiltersTest, HiddenOnly) {
    auto entries = sample_entries();
    auto result = entries | hidden_only;
    EXPECT_EQ(std::ranges::distance(result), 1);
}

TEST(FiltersTest, NoHidden) {
    auto entries = sample_entries();
    auto result = entries | no_hidden;
    EXPECT_EQ(std::ranges::distance(result), 5);
}

TEST(FiltersTest, TypeIs) {
    auto entries = sample_entries();
    auto result = entries | type_is(file_type::directory);
    EXPECT_EQ(std::ranges::distance(result), 1);
}

// ── Size filters ────────────────────────────────────────────────────────

TEST(FiltersTest, SizeGt) {
    auto entries = sample_entries();
    auto result = entries | size_gt(1024);
    EXPECT_EQ(std::ranges::distance(result), 2); // main.cpp=4096, lib.hpp=2048
}

TEST(FiltersTest, SizeLt) {
    auto entries = sample_entries();
    auto result = entries | size_lt(1024);
    EXPECT_EQ(std::ranges::distance(result), 3); // build=0, .gitignore=256, empty.txt=0
}

TEST(FiltersTest, SizeEq) {
    auto entries = sample_entries();
    auto result = entries | size_eq(1024);
    EXPECT_EQ(std::ranges::distance(result), 1); // readme.md
}

TEST(FiltersTest, SizeRange) {
    auto entries = sample_entries();
    auto result = entries | size_range(256, 2048);
    EXPECT_EQ(std::ranges::distance(result), 3); // readme=1024, .gitignore=256, lib=2048
}

TEST(FiltersTest, EmptyFiles) {
    auto entries = sample_entries();
    auto result = entries | empty_files;
    EXPECT_EQ(std::ranges::distance(result), 1); // empty.txt (build is dir, not regular)
}

// ── Name/Extension filters ──────────────────────────────────────────────

TEST(FiltersTest, ExtensionIs) {
    auto entries = sample_entries();
    auto result = entries | extension_is("cpp");
    EXPECT_EQ(std::ranges::distance(result), 1); // main.cpp
}

TEST(FiltersTest, ExtensionIsDot) {
    auto entries = sample_entries();
    auto result = entries | extension_is(".hpp");
    EXPECT_EQ(std::ranges::distance(result), 1); // lib.hpp
}

TEST(FiltersTest, ExtensionIn) {
    auto entries = sample_entries();
    auto result = entries | extension_in({"cpp", "hpp"});
    EXPECT_EQ(std::ranges::distance(result), 2);
}

TEST(FiltersTest, NameContains) {
    auto entries = sample_entries();
    auto result = entries | name_contains("main");
    EXPECT_EQ(std::ranges::distance(result), 1);
}

TEST(FiltersTest, NameStartsWith) {
    auto entries = sample_entries();
    auto result = entries | name_starts_with("lib");
    EXPECT_EQ(std::ranges::distance(result), 1);
}

TEST(FiltersTest, NameEndsWith) {
    auto entries = sample_entries();
    auto result = entries | name_ends_with(".md");
    EXPECT_EQ(std::ranges::distance(result), 1);
}

TEST(FiltersTest, NameEq) {
    auto entries = sample_entries();
    auto result = entries | name_eq("build");
    EXPECT_EQ(std::ranges::distance(result), 1);
}

TEST(FiltersTest, NameIcase) {
    auto entries = sample_entries();
    auto result = entries | name_icase("MAIN.CPP");
    EXPECT_EQ(std::ranges::distance(result), 1);
}

TEST(FiltersTest, StemEq) {
    auto entries = sample_entries();
    auto result = entries | stem_eq("main");
    EXPECT_EQ(std::ranges::distance(result), 1);
}

// ── Boolean combinators ─────────────────────────────────────────────────

TEST(FiltersTest, AndCombinator) {
    auto entries = sample_entries();
    auto f = files_only && size_gt(1000);
    auto result = entries | f;
    EXPECT_EQ(std::ranges::distance(result), 3); // readme=1024, main=4096, lib=2048
}

TEST(FiltersTest, OrCombinator) {
    auto entries = sample_entries();
    auto f = extension_is("cpp") || extension_is("hpp");
    auto result = entries | f;
    EXPECT_EQ(std::ranges::distance(result), 2);
}

TEST(FiltersTest, NotCombinator) {
    auto entries = sample_entries();
    auto f = !hidden_only;
    auto result = entries | f;
    EXPECT_EQ(std::ranges::distance(result), 5);
}

// ── Permission filters ──────────────────────────────────────────────────

TEST(FiltersTest, Readable) {
    auto entries = sample_entries();
    auto result = entries | readable;
    EXPECT_EQ(std::ranges::distance(result), 6); // all have owner_read
}

TEST(FiltersTest, HasPermission) {
    auto entries = sample_entries();
    auto result = entries | has_permission(perms::owner_write);
    EXPECT_EQ(std::ranges::distance(result), 6);
}

// ── Link filters ────────────────────────────────────────────────────────

TEST(FiltersTest, LinksEq) {
    auto entries = sample_entries();
    auto result = entries | links_eq(1);
    EXPECT_EQ(std::ranges::distance(result), 6);
}
