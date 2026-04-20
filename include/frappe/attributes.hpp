#ifndef FRAPPE_ATTRIBUTES_HPP
#define FRAPPE_ATTRIBUTES_HPP

#include "frappe/error.hpp"
#include "frappe/exports.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace frappe {

using path = std::filesystem::path;

// Extended attributes (xattr)
class FRAPPE_API extended_attributes {
  public:
    explicit extended_attributes(const path &p) noexcept : _path(p) {}

    [[nodiscard]] result<bool> has(std::string_view name) const noexcept;
    [[nodiscard]] result<std::vector<std::uint8_t>> get(std::string_view name) const noexcept;
    [[nodiscard]] result<std::string> get_string(std::string_view name) const noexcept;
    [[nodiscard]] result<std::vector<std::string>> list() const noexcept;
    [[nodiscard]] result<std::size_t> size(std::string_view name) const noexcept;
    [[nodiscard]] void_result set(std::string_view name, std::span<const std::uint8_t> value) noexcept;
    [[nodiscard]] void_result set(std::string_view name, std::string_view value) noexcept;
    [[nodiscard]] void_result remove(std::string_view name) noexcept;

    [[nodiscard]] const path &file_path() const noexcept { return _path; }

  private:
    path _path;
};

// Convenience functions
[[nodiscard]] FRAPPE_API result<bool> has_xattr(const path &p, std::string_view name) noexcept;
[[nodiscard]] FRAPPE_API result<std::vector<std::uint8_t>> get_xattr(const path &p, std::string_view name) noexcept;
[[nodiscard]] FRAPPE_API result<std::string> get_xattr_string(const path &p, std::string_view name) noexcept;
[[nodiscard]] FRAPPE_API result<std::vector<std::string>> list_xattr(const path &p) noexcept;
[[nodiscard]] FRAPPE_API void_result set_xattr(const path &p, std::string_view name,
                                               std::span<const std::uint8_t> value) noexcept;
[[nodiscard]] FRAPPE_API void_result set_xattr(const path &p, std::string_view name, std::string_view value) noexcept;
[[nodiscard]] FRAPPE_API void_result remove_xattr(const path &p, std::string_view name) noexcept;

// Check if xattr is supported on the filesystem containing the given path
[[nodiscard]] FRAPPE_API result<bool> xattr_supported(const path &p) noexcept;

// MIME type detection
[[nodiscard]] FRAPPE_API result<std::string> detect_mime_type(const path &p) noexcept;
[[nodiscard]] FRAPPE_API std::string mime_type_from_extension(std::string_view ext) noexcept;
[[nodiscard]] FRAPPE_API result<std::string> mime_type_from_content(const path &p) noexcept;
[[nodiscard]] FRAPPE_API std::string mime_type_from_content(std::span<const std::uint8_t> data) noexcept;
[[nodiscard]] FRAPPE_API std::string extension_from_mime_type(std::string_view mime) noexcept;

// File type identification (magic number detection)
enum class file_category { unknown, text, binary, image, audio, video, archive, document, executable, script, data };

struct file_type_info {
    file_category category = file_category::unknown;
    std::string mime_type;
    std::string description;
    std::string extension;
};

[[nodiscard]] FRAPPE_API result<file_type_info> identify_file_type(const path &p) noexcept;
[[nodiscard]] FRAPPE_API file_type_info identify_file_type(std::span<const std::uint8_t> data) noexcept;

[[nodiscard]] FRAPPE_API result<bool> is_text_file(const path &p) noexcept;
[[nodiscard]] FRAPPE_API result<bool> is_binary_file(const path &p) noexcept;

[[nodiscard]] FRAPPE_API std::string_view file_category_name(file_category cat) noexcept;

// Special file info (5.5)
[[nodiscard]] FRAPPE_API result<std::uint64_t> device_id(const path &p) noexcept;
[[nodiscard]] FRAPPE_API result<std::uint64_t> inode(const path &p) noexcept;      // POSIX inode
[[nodiscard]] FRAPPE_API result<std::uint64_t> file_index(const path &p) noexcept; // Windows file index

// Windows-specific: Alternate Data Streams (ADS)
#ifdef _WIN32
[[nodiscard]] FRAPPE_API result<std::vector<std::string>> list_alternate_streams(const path &p) noexcept;
[[nodiscard]] FRAPPE_API result<bool> has_alternate_stream(const path &p, std::string_view stream_name) noexcept;
#endif

} // namespace frappe

#endif // FRAPPE_ATTRIBUTES_HPP
