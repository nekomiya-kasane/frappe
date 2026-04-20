#include "frappe/path.hpp"

#include <filesystem>
#include <gtest/gtest.h>

TEST(PathTest, HomePath) {
    auto home = frappe::home_path();
    EXPECT_TRUE(home.has_value());
    EXPECT_FALSE(home->empty());
    EXPECT_TRUE(std::filesystem::exists(*home));
    EXPECT_TRUE(std::filesystem::is_directory(*home));
}

TEST(PathTest, ExecutablePath) {
    auto exe = frappe::executable_path();
    EXPECT_TRUE(exe.has_value());
    EXPECT_FALSE(exe->empty());
    EXPECT_TRUE(std::filesystem::exists(*exe));
    EXPECT_TRUE(std::filesystem::is_regular_file(*exe));
}

TEST(PathTest, AppDataPath) {
    auto app_data = frappe::app_data_path();
    EXPECT_TRUE(app_data.has_value());
    EXPECT_FALSE(app_data->empty());
    EXPECT_TRUE(std::filesystem::exists(*app_data));
}

TEST(PathTest, DesktopPath) {
    auto desktop = frappe::desktop_path();
    EXPECT_TRUE(desktop.has_value());
    EXPECT_FALSE(desktop->empty());
}

TEST(PathTest, DocumentsPath) {
    auto docs = frappe::documents_path();
    EXPECT_TRUE(docs.has_value());
    EXPECT_FALSE(docs->empty());
}

TEST(PathTest, DownloadsPath) {
    auto downloads = frappe::downloads_path();
    EXPECT_TRUE(downloads.has_value());
    EXPECT_FALSE(downloads->empty());
}

TEST(PathTest, ResolvePathTilde) {
    auto resolved = frappe::resolve_path("~");
    EXPECT_TRUE(resolved.has_value());
    auto home = frappe::home_path();
    EXPECT_EQ(*resolved, *home);
}

TEST(PathTest, ResolvePathTildeSubdir) {
    auto resolved = frappe::resolve_path("~/test");
    EXPECT_TRUE(resolved.has_value());
    auto home = frappe::home_path();
    EXPECT_EQ(*resolved, *home / "test");
}

TEST(PathTest, ResolvePathEnvVar) {
#ifdef _WIN32
    auto resolved = frappe::resolve_path("$USERPROFILE");
#else
    auto resolved = frappe::resolve_path("$HOME");
#endif
    EXPECT_TRUE(resolved.has_value());
    EXPECT_FALSE(resolved->empty());
}

TEST(PathTest, ResolvePathEnvVarBraces) {
#ifdef _WIN32
    auto resolved = frappe::resolve_path("${USERPROFILE}");
#else
    auto resolved = frappe::resolve_path("${HOME}");
#endif
    EXPECT_TRUE(resolved.has_value());
    EXPECT_FALSE(resolved->empty());
}

TEST(PathTest, ResolvePathNoChange) {
    auto resolved = frappe::resolve_path("/some/absolute/path");
    EXPECT_TRUE(resolved.has_value());
    EXPECT_EQ(resolved->string(), "/some/absolute/path");
}
