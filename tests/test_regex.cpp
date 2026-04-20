#include "frappe/regex.hpp"

#include <filesystem>
#include <gtest/gtest.h>
#include <vector>

TEST(RegexTest, CompileValid) {
    EXPECT_NO_THROW({
        auto re = frappe::compile_regex("hello.*world");
        EXPECT_TRUE(re.valid());
    });
}

TEST(RegexTest, CompileInvalid) {
    EXPECT_THROW({ auto re = frappe::compile_regex("[invalid"); }, frappe::regex_error);
}

TEST(RegexTest, MatchSimple) {
    auto re = frappe::compile_regex("hello");
    EXPECT_TRUE(re.match("hello"));
    EXPECT_FALSE(re.match("hello world"));
    EXPECT_FALSE(re.match("world"));
}

TEST(RegexTest, MatchPattern) {
    auto re = frappe::compile_regex("file[0-9]+\\.txt");
    EXPECT_TRUE(re.match("file123.txt"));
    EXPECT_TRUE(re.match("file1.txt"));
    EXPECT_FALSE(re.match("file.txt"));
    EXPECT_FALSE(re.match("fileabc.txt"));
}

TEST(RegexTest, MatchCaseless) {
    auto re = frappe::compile_regex("HELLO", frappe::regex_options::caseless);
    EXPECT_TRUE(re.match("hello"));
    EXPECT_TRUE(re.match("HELLO"));
    EXPECT_TRUE(re.match("HeLLo"));
}

TEST(RegexTest, SearchSimple) {
    auto re = frappe::compile_regex("world");
    EXPECT_TRUE(re.search("hello world"));
    EXPECT_TRUE(re.search("world"));
    EXPECT_FALSE(re.search("hello"));
}

TEST(RegexTest, ReplaceSimple) {
    auto re = frappe::compile_regex("world");
    auto result = re.replace("hello world", "universe");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "hello universe");
}

TEST(RegexTest, ReplaceMultiple) {
    auto re = frappe::compile_regex("o");
    auto result = re.replace("hello world", "0");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "hell0 w0rld");
}

TEST(RegexTest, RegexMatchPath) {
    frappe::path p("/path/to/file123.txt");
    EXPECT_TRUE(frappe::regex_match(p, ".*file[0-9]+\\.txt"));
    EXPECT_FALSE(frappe::regex_match(p, ".*file[a-z]+\\.txt"));
}

TEST(RegexTest, RegexMatchString) {
    EXPECT_TRUE(frappe::regex_match(std::string_view("test.cpp"), std::string_view(".*\\.cpp")));
    EXPECT_FALSE(frappe::regex_match(std::string_view("test.hpp"), std::string_view(".*\\.cpp")));
}

TEST(RegexTest, RegexSearchPath) {
    frappe::path p("/path/to/file.txt");
    EXPECT_TRUE(frappe::regex_search(p, "file"));
    EXPECT_FALSE(frappe::regex_search(p, "missing"));
}

TEST(RegexTest, RegexReplacePath) {
    frappe::path p("/path/to/old_file.txt");
    auto result = frappe::regex_replace(p, "old", "new");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->string(), "/path/to/new_file.txt");
}

TEST(RegexTest, RegexReplaceString) {
    auto result =
        frappe::regex_replace(std::string_view("hello_world.txt"), std::string_view("_"), std::string_view("-"));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "hello-world.txt");
}

TEST(RegexTest, FilterRegex) {
    std::vector<frappe::path> paths = {"/path/file1.cpp", "/path/file2.hpp", "/path/file3.cpp", "/path/file4.txt"};

    auto filtered = paths | frappe::filter_regex(".*\\.cpp");

    std::vector<frappe::path> result;
    for (const auto &p : filtered) {
        result.push_back(p);
    }

    EXPECT_EQ(result.size(), 2u);
}

TEST(RegexTest, FilterRegexNot) {
    std::vector<frappe::path> paths = {"/path/file1.cpp", "/path/file2.hpp", "/path/file3.cpp", "/path/file4.txt"};

    auto filtered = paths | frappe::filter_regex_not(".*\\.cpp");

    std::vector<frappe::path> result;
    for (const auto &p : filtered) {
        result.push_back(p);
    }

    EXPECT_EQ(result.size(), 2u);
}

TEST(RegexTest, PatternGetter) {
    auto re = frappe::compile_regex("test.*pattern");
    EXPECT_EQ(re.pattern(), "test.*pattern");
}

TEST(RegexTest, MoveSemantics) {
    auto re1 = frappe::compile_regex("hello");
    EXPECT_TRUE(re1.valid());

    auto re2 = std::move(re1);
    EXPECT_TRUE(re2.valid());
    EXPECT_TRUE(re2.match("hello"));
}
