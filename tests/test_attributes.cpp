#include "frappe/attributes.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

class AttributesTest : public ::testing::Test {
  protected:
    void SetUp() override {
        _test_dir = std::filesystem::temp_directory_path() / "frappe_attributes_test";
        std::filesystem::create_directories(_test_dir);

        _test_file = _test_dir / "test_file.txt";
        std::ofstream(_test_file) << "test content for attributes";

        _png_file = _test_dir / "test.png";
        std::ofstream png_ofs(_png_file, std::ios::binary);
        unsigned char png_header[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
        png_ofs.write(reinterpret_cast<char *>(png_header), sizeof(png_header));

        _zip_file = _test_dir / "test.zip";
        std::ofstream zip_ofs(_zip_file, std::ios::binary);
        unsigned char zip_header[] = {0x50, 0x4B, 0x03, 0x04};
        zip_ofs.write(reinterpret_cast<char *>(zip_header), sizeof(zip_header));

        _pdf_file = _test_dir / "test.pdf";
        std::ofstream pdf_ofs(_pdf_file, std::ios::binary);
        pdf_ofs << "%PDF-1.4";
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(_test_dir, ec);
    }

    std::filesystem::path _test_dir;
    std::filesystem::path _test_file;
    std::filesystem::path _png_file;
    std::filesystem::path _zip_file;
    std::filesystem::path _pdf_file;
};

TEST_F(AttributesTest, XattrSupported) {
    auto supported = frappe::xattr_supported(_test_file);
    ASSERT_TRUE(supported.has_value());
    EXPECT_TRUE(*supported);
}

TEST_F(AttributesTest, XattrSupportedForPath) {
    auto supported = frappe::xattr_supported(_test_file);
    (void)supported;
}

TEST_F(AttributesTest, ListXattrEmpty) {
    auto attrs = frappe::list_xattr(_test_file);
    EXPECT_TRUE(!attrs.has_value() || attrs->empty());
}

TEST_F(AttributesTest, ExtendedAttributesClass) {
    frappe::extended_attributes xa(_test_file);
    EXPECT_EQ(xa.file_path(), _test_file);
}

TEST_F(AttributesTest, MimeTypeFromExtension) {
    EXPECT_EQ(frappe::mime_type_from_extension(".txt"), "text/plain");
    EXPECT_EQ(frappe::mime_type_from_extension(".html"), "text/html");
    EXPECT_EQ(frappe::mime_type_from_extension(".jpg"), "image/jpeg");
    EXPECT_EQ(frappe::mime_type_from_extension(".png"), "image/png");
    EXPECT_EQ(frappe::mime_type_from_extension(".pdf"), "application/pdf");
    EXPECT_EQ(frappe::mime_type_from_extension(".zip"), "application/zip");
    EXPECT_EQ(frappe::mime_type_from_extension(".mp3"), "audio/mpeg");
    EXPECT_EQ(frappe::mime_type_from_extension(".mp4"), "video/mp4");
}

TEST_F(AttributesTest, MimeTypeFromExtensionWithoutDot) {
    EXPECT_EQ(frappe::mime_type_from_extension("txt"), "text/plain");
    EXPECT_EQ(frappe::mime_type_from_extension("png"), "image/png");
}

TEST_F(AttributesTest, MimeTypeFromExtensionUnknown) {
    EXPECT_EQ(frappe::mime_type_from_extension(".xyz123"), "application/octet-stream");
}

TEST_F(AttributesTest, ExtensionFromMimeType) {
    EXPECT_EQ(frappe::extension_from_mime_type("text/plain"), ".txt");
    EXPECT_EQ(frappe::extension_from_mime_type("image/png"), ".png");
    EXPECT_EQ(frappe::extension_from_mime_type("application/pdf"), ".pdf");
}

TEST_F(AttributesTest, ExtensionFromMimeTypeUnknown) {
    EXPECT_EQ(frappe::extension_from_mime_type("application/x-unknown"), "");
}

TEST_F(AttributesTest, IdentifyTextFile) {
    auto info = frappe::identify_file_type(_test_file);
    EXPECT_TRUE(info.has_value());
    EXPECT_EQ(info->category, frappe::file_category::text);
}

TEST_F(AttributesTest, IdentifyPngFile) {
    auto info = frappe::identify_file_type(_png_file);
    EXPECT_TRUE(info.has_value());
    EXPECT_EQ(info->category, frappe::file_category::image);
    EXPECT_EQ(info->mime_type, "image/png");
}

TEST_F(AttributesTest, IdentifyZipFile) {
    auto info = frappe::identify_file_type(_zip_file);
    EXPECT_TRUE(info.has_value());
    EXPECT_EQ(info->category, frappe::file_category::archive);
    EXPECT_EQ(info->mime_type, "application/zip");
}

TEST_F(AttributesTest, IdentifyPdfFile) {
    auto info = frappe::identify_file_type(_pdf_file);
    EXPECT_TRUE(info.has_value());
    EXPECT_EQ(info->category, frappe::file_category::document);
    EXPECT_EQ(info->mime_type, "application/pdf");
}

TEST_F(AttributesTest, IdentifyFromData) {
    std::vector<std::uint8_t> png_data = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    auto info = frappe::identify_file_type(png_data);
    EXPECT_EQ(info.category, frappe::file_category::image);
    EXPECT_EQ(info.mime_type, "image/png");
}

TEST_F(AttributesTest, IsTextFile) {
    auto r1 = frappe::is_text_file(_test_file);
    EXPECT_TRUE(r1.has_value() && *r1);
    auto r2 = frappe::is_text_file(_png_file);
    EXPECT_TRUE(r2.has_value() && !*r2);
}

TEST_F(AttributesTest, IsBinaryFile) {
    auto r1 = frappe::is_binary_file(_test_file);
    EXPECT_TRUE(r1.has_value() && !*r1);
    auto r2 = frappe::is_binary_file(_png_file);
    EXPECT_TRUE(r2.has_value() && *r2);
}

TEST_F(AttributesTest, DetectMimeType) {
    auto mime = frappe::detect_mime_type(_png_file);
    EXPECT_TRUE(mime.has_value());
    EXPECT_EQ(*mime, "image/png");
}

TEST_F(AttributesTest, FileCategoryName) {
    EXPECT_EQ(frappe::file_category_name(frappe::file_category::text), "text");
    EXPECT_EQ(frappe::file_category_name(frappe::file_category::image), "image");
    EXPECT_EQ(frappe::file_category_name(frappe::file_category::audio), "audio");
    EXPECT_EQ(frappe::file_category_name(frappe::file_category::video), "video");
    EXPECT_EQ(frappe::file_category_name(frappe::file_category::archive), "archive");
    EXPECT_EQ(frappe::file_category_name(frappe::file_category::document), "document");
    EXPECT_EQ(frappe::file_category_name(frappe::file_category::executable), "executable");
    EXPECT_EQ(frappe::file_category_name(frappe::file_category::unknown), "unknown");
}

#ifdef _WIN32
TEST_F(AttributesTest, SetAndGetXattr) {
    auto supported = frappe::xattr_supported(_test_file);
    if (!supported || !*supported) {
        GTEST_SKIP() << "xattr not supported on this filesystem";
    }

    std::string test_value = "test_xattr_value";
    auto set_result = frappe::set_xattr(_test_file, "test_attr", test_value);
    if (!set_result) {
        GTEST_SKIP() << "Failed to set xattr";
    }

    auto has_result = frappe::has_xattr(_test_file, "test_attr");
    EXPECT_TRUE(has_result.has_value() && *has_result);

    auto value = frappe::get_xattr_string(_test_file, "test_attr");
    EXPECT_TRUE(value.has_value());
    EXPECT_EQ(*value, test_value);

    auto attrs = frappe::list_xattr(_test_file);
    EXPECT_TRUE(attrs.has_value());
    EXPECT_FALSE(attrs->empty());

    auto remove_result = frappe::remove_xattr(_test_file, "test_attr");
    EXPECT_TRUE(remove_result.has_value());

    auto has_after = frappe::has_xattr(_test_file, "test_attr");
    EXPECT_TRUE(has_after.has_value() && !*has_after);
}

TEST_F(AttributesTest, AlternateDataStreams) {
    auto streams = frappe::list_alternate_streams(_test_file);
    (void)streams;
}
#endif

TEST_F(AttributesTest, IdentifyEmptyData) {
    std::vector<std::uint8_t> empty_data;
    auto info = frappe::identify_file_type(empty_data);
    EXPECT_EQ(info.category, frappe::file_category::unknown);
}

TEST_F(AttributesTest, MimeTypeCaseInsensitive) {
    EXPECT_EQ(frappe::mime_type_from_extension(".TXT"), "text/plain");
    EXPECT_EQ(frappe::mime_type_from_extension(".PNG"), "image/png");
    EXPECT_EQ(frappe::mime_type_from_extension(".Jpg"), "image/jpeg");
}

TEST_F(AttributesTest, DeviceId) {
    auto dev = frappe::device_id(_test_file);
    EXPECT_TRUE(dev.has_value());
    EXPECT_GT(*dev, 0u);
}

TEST_F(AttributesTest, Inode) {
    auto ino = frappe::inode(_test_file);
    EXPECT_TRUE(ino.has_value());
    EXPECT_GT(*ino, 0u);
}

TEST_F(AttributesTest, FileIndex) {
    auto idx = frappe::file_index(_test_file);
    EXPECT_TRUE(idx.has_value());
    EXPECT_GT(*idx, 0u);
}

TEST_F(AttributesTest, InodeEqualsFileIndex) {
    auto ino = frappe::inode(_test_file);
    auto idx = frappe::file_index(_test_file);
    EXPECT_TRUE(ino.has_value());
    EXPECT_TRUE(idx.has_value());
    EXPECT_EQ(*ino, *idx);
}

// 5.3 MIME type from content
TEST_F(AttributesTest, MimeTypeFromContentPath) {
    auto mime = frappe::mime_type_from_content(_test_file);
    EXPECT_TRUE(mime.has_value());
}

TEST_F(AttributesTest, MimeTypeFromContentData) {
    // PNG magic bytes
    std::vector<std::uint8_t> png_data = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    auto mime = frappe::mime_type_from_content(png_data);
    EXPECT_EQ(mime, "image/png");

    // JPEG magic bytes
    std::vector<std::uint8_t> jpg_data = {0xFF, 0xD8, 0xFF, 0xE0};
    mime = frappe::mime_type_from_content(jpg_data);
    EXPECT_EQ(mime, "image/jpeg");

    // Unknown data
    std::vector<std::uint8_t> unknown_data = {0x00, 0x01, 0x02, 0x03};
    mime = frappe::mime_type_from_content(unknown_data);
    EXPECT_EQ(mime, "application/octet-stream");
}
