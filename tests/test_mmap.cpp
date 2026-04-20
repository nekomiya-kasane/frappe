#include "frappe/mmap.hpp"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

class MmapTest : public ::testing::Test {
  protected:
    fs::path tmp_dir_;

    void SetUp() override {
        tmp_dir_ = fs::temp_directory_path() / "frappe_mmap_test";
        fs::create_directories(tmp_dir_);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(tmp_dir_, ec);
    }

    fs::path make_file(const std::string &name, const std::string &content) {
        auto p = tmp_dir_ / name;
        std::ofstream f(p, std::ios::binary);
        f.write(content.data(), content.size());
        return p;
    }

    fs::path make_binary_file(const std::string &name, size_t size, uint8_t seed = 0) {
        auto p = tmp_dir_ / name;
        std::ofstream f(p, std::ios::binary);
        for (size_t i = 0; i < size; ++i) {
            auto b = static_cast<char>((i + seed) & 0xFF);
            f.write(&b, 1);
        }
        return p;
    }
};

TEST_F(MmapTest, OpenReadOnly) {
    auto p = make_file("read.txt", "Hello, mmap!");
    auto result = frappe::mapped_file::open(p, frappe::map_mode::read_only);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    auto &mf = *result;
    EXPECT_TRUE(mf.is_open());
    EXPECT_EQ(mf.size(), 12u);
    EXPECT_EQ(mf.as_string(), "Hello, mmap!");
    EXPECT_EQ(mf.mode(), frappe::map_mode::read_only);
}

TEST_F(MmapTest, OpenNonExistent) {
    auto result = frappe::mapped_file::open(tmp_dir_ / "nonexistent.txt");
    EXPECT_FALSE(result.has_value());
}

TEST_F(MmapTest, EmptyFile) {
    auto p = make_file("empty.txt", "");
    auto result = frappe::mapped_file::open(p);
    // Empty files may or may not be mappable depending on platform
    if (result.has_value()) {
        EXPECT_EQ(result->size(), 0u);
        EXPECT_TRUE(result->empty());
    }
}

TEST_F(MmapTest, ReadWriteMode) {
    auto p = make_file("rw.txt", "ABCDEFGH");
    {
        auto result = frappe::mapped_file::open(p, frappe::map_mode::read_write);
        ASSERT_TRUE(result.has_value()) << result.error().message();
        auto &mf = *result;
        EXPECT_EQ(mf.size(), 8u);
        // Modify through mapping
        auto *ptr = mf.data();
        ASSERT_NE(ptr, nullptr);
        ptr[0] = std::byte{'X'};
        ptr[7] = std::byte{'Y'};
        mf.sync();
    }
    // Verify changes persisted
    std::ifstream f(p, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(f)), {});
    EXPECT_EQ(content, "XBCDEFGY");
}

TEST_F(MmapTest, TypedAccess) {
    auto p = make_binary_file("typed.bin", 16, 0);
    auto result = frappe::mapped_file::open(p);
    ASSERT_TRUE(result.has_value());
    auto ints = result->as<uint32_t>();
    EXPECT_EQ(ints.size(), 4u);
}

TEST_F(MmapTest, ConvenienceMmapRead) {
    auto p = make_file("conv.txt", "convenience");
    auto result = frappe::mmap_read(p);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->as_string(), "convenience");
}

TEST_F(MmapTest, MappedView) {
    auto p = make_file("view.txt", "Hello World!");
    auto result = frappe::mapped_file::open(p);
    ASSERT_TRUE(result.has_value());

    frappe::mapped_view view(*result);
    EXPECT_EQ(view.size(), 12u);
    EXPECT_EQ(view.as_string(), "Hello World!");

    auto sub = view.subview(6, 5);
    EXPECT_EQ(sub.as_string(), "World");
}

TEST_F(MmapTest, MappedViewSubview) {
    auto p = make_file("subview.txt", "0123456789");
    auto result = frappe::mapped_file::open(p);
    ASSERT_TRUE(result.has_value());

    frappe::mapped_view view(*result);
    auto sub = view.subview(3, 4);
    EXPECT_EQ(sub.size(), 4u);
    EXPECT_EQ(sub.as_string(), "3456");

    // Subview beyond end
    auto sub2 = view.subview(8, 100);
    EXPECT_EQ(sub2.size(), 2u);
    EXPECT_EQ(sub2.as_string(), "89");

    // Subview at end
    auto sub3 = view.subview(10, 5);
    EXPECT_TRUE(sub3.empty());
}

TEST_F(MmapTest, CreateNewFile) {
    auto p = tmp_dir_ / "created.bin";
    auto result = frappe::mapped_file::create(p, 1024);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_EQ(result->size(), 1024u);
    EXPECT_TRUE(result->is_open());
    result->close();
    EXPECT_EQ(fs::file_size(p), 1024u);
}

TEST_F(MmapTest, MoveSemantics) {
    auto p = make_file("move.txt", "movable");
    auto result = frappe::mapped_file::open(p);
    ASSERT_TRUE(result.has_value());

    frappe::mapped_file mf2 = std::move(*result);
    EXPECT_TRUE(mf2.is_open());
    EXPECT_EQ(mf2.as_string(), "movable");
}

TEST_F(MmapTest, LargerFile) {
    auto p = make_binary_file("large.bin", 100000, 0x42);
    auto result = frappe::mapped_file::open(p);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 100000u);

    // Verify content
    auto *data = reinterpret_cast<const uint8_t *>(result->data());
    for (size_t i = 0; i < 100; ++i) {
        EXPECT_EQ(data[i], static_cast<uint8_t>((i + 0x42) & 0xFF));
    }
}

TEST_F(MmapTest, PartialMapping) {
    auto p = make_file("partial.txt", "Hello, World! This is a test.");
    auto result = frappe::mapped_file::open(p, 7, 6);
    if (result.has_value()) {
        EXPECT_EQ(result->size(), 6u);
        EXPECT_EQ(result->as_string(), "World!");
    }
}
