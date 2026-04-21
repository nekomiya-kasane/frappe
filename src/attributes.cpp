#include "frappe/attributes.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <unordered_map>

namespace frappe {

    namespace detail {
        result<bool> has_xattr_impl(const path &p, std::string_view name) noexcept;
        result<std::vector<std::uint8_t>> get_xattr_impl(const path &p, std::string_view name) noexcept;
        result<std::vector<std::string>> list_xattr_impl(const path &p) noexcept;
        result<std::size_t> size_xattr_impl(const path &p, std::string_view name) noexcept;
        void_result set_xattr_impl(const path &p, std::string_view name, std::span<const std::uint8_t> value) noexcept;
        void_result remove_xattr_impl(const path &p, std::string_view name) noexcept;
        result<bool> xattr_supported_impl(const path &p) noexcept;
    } // namespace detail

    // Extended attributes class implementation
    result<bool> extended_attributes::has(std::string_view name) const noexcept {
        return detail::has_xattr_impl(_path, name);
    }

    result<std::vector<std::uint8_t>> extended_attributes::get(std::string_view name) const noexcept {
        return detail::get_xattr_impl(_path, name);
    }

    result<std::string> extended_attributes::get_string(std::string_view name) const noexcept {
        auto data = get(name);
        if (!data) {
            return std::unexpected(data.error());
        }
        return std::string(data->begin(), data->end());
    }

    result<std::vector<std::string>> extended_attributes::list() const noexcept {
        return detail::list_xattr_impl(_path);
    }

    result<std::size_t> extended_attributes::size(std::string_view name) const noexcept {
        return detail::size_xattr_impl(_path, name);
    }

    void_result extended_attributes::set(std::string_view name, std::span<const std::uint8_t> value) noexcept {
        return detail::set_xattr_impl(_path, name, value);
    }

    void_result extended_attributes::set(std::string_view name, std::string_view value) noexcept {
        std::vector<std::uint8_t> data(value.begin(), value.end());
        return set(name, data);
    }

    void_result extended_attributes::remove(std::string_view name) noexcept {
        return detail::remove_xattr_impl(_path, name);
    }

    // Convenience functions
    result<bool> has_xattr(const path &p, std::string_view name) noexcept {
        return extended_attributes(p).has(name);
    }

    result<std::vector<std::uint8_t>> get_xattr(const path &p, std::string_view name) noexcept {
        return extended_attributes(p).get(name);
    }

    result<std::string> get_xattr_string(const path &p, std::string_view name) noexcept {
        return extended_attributes(p).get_string(name);
    }

    result<std::vector<std::string>> list_xattr(const path &p) noexcept {
        return extended_attributes(p).list();
    }

    void_result set_xattr(const path &p, std::string_view name, std::span<const std::uint8_t> value) noexcept {
        return extended_attributes(p).set(name, value);
    }

    void_result set_xattr(const path &p, std::string_view name, std::string_view value) noexcept {
        return extended_attributes(p).set(name, value);
    }

    void_result remove_xattr(const path &p, std::string_view name) noexcept {
        return extended_attributes(p).remove(name);
    }

    result<bool> xattr_supported(const path &p) noexcept {
        return detail::xattr_supported_impl(p);
    }

    // MIME type detection
    namespace {
        const std::unordered_map<std::string, std::string> &get_extension_mime_map() {
            static const std::unordered_map<std::string, std::string> map = {
                // Text
                {".txt", "text/plain"},
                {".html", "text/html"},
                {".htm", "text/html"},
                {".css", "text/css"},
                {".js", "text/javascript"},
                {".json", "application/json"},
                {".xml", "text/xml"},
                {".csv", "text/csv"},
                {".md", "text/markdown"},
                {".yaml", "text/yaml"},
                {".yml", "text/yaml"},

                // Images
                {".jpg", "image/jpeg"},
                {".jpeg", "image/jpeg"},
                {".png", "image/png"},
                {".gif", "image/gif"},
                {".bmp", "image/bmp"},
                {".webp", "image/webp"},
                {".svg", "image/svg+xml"},
                {".ico", "image/x-icon"},
                {".tiff", "image/tiff"},
                {".tif", "image/tiff"},

                // Audio
                {".mp3", "audio/mpeg"},
                {".wav", "audio/wav"},
                {".ogg", "audio/ogg"},
                {".flac", "audio/flac"},
                {".aac", "audio/aac"},
                {".m4a", "audio/mp4"},
                {".wma", "audio/x-ms-wma"},

                // Video
                {".mp4", "video/mp4"},
                {".avi", "video/x-msvideo"},
                {".mkv", "video/x-matroska"},
                {".mov", "video/quicktime"},
                {".wmv", "video/x-ms-wmv"},
                {".flv", "video/x-flv"},
                {".webm", "video/webm"},

                // Archives
                {".zip", "application/zip"},
                {".rar", "application/vnd.rar"},
                {".7z", "application/x-7z-compressed"},
                {".tar", "application/x-tar"},
                {".gz", "application/gzip"},
                {".bz2", "application/x-bzip2"},
                {".xz", "application/x-xz"},

                // Documents
                {".pdf", "application/pdf"},
                {".doc", "application/msword"},
                {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
                {".xls", "application/vnd.ms-excel"},
                {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
                {".ppt", "application/vnd.ms-powerpoint"},
                {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
                {".odt", "application/vnd.oasis.opendocument.text"},
                {".ods", "application/vnd.oasis.opendocument.spreadsheet"},

                // Executables
                {".exe", "application/x-msdownload"},
                {".dll", "application/x-msdownload"},
                {".so", "application/x-sharedlib"},
                {".dylib", "application/x-mach-binary"},

                // Programming
                {".c", "text/x-c"},
                {".cpp", "text/x-c++"},
                {".h", "text/x-c"},
                {".hpp", "text/x-c++"},
                {".py", "text/x-python"},
                {".java", "text/x-java"},
                {".rs", "text/x-rust"},
                {".go", "text/x-go"},
                {".rb", "text/x-ruby"},
                {".php", "text/x-php"},
                {".sh", "text/x-shellscript"},
                {".bat", "text/x-batch"},
                {".ps1", "text/x-powershell"},

                // Data
                {".sql", "application/sql"},
                {".db", "application/x-sqlite3"},
                {".sqlite", "application/x-sqlite3"},
            };
            return map;
        }

        const std::unordered_map<std::string, std::string> &get_mime_extension_map() {
            static std::unordered_map<std::string, std::string> map;
            static bool initialized = false;
            if (!initialized) {
                for (const auto &[ext, mime] : get_extension_mime_map()) {
                    if (map.find(mime) == map.end()) {
                        map[mime] = ext;
                    }
                }
                initialized = true;
            }
            return map;
        }
    } // namespace

    std::string mime_type_from_extension(std::string_view ext) noexcept {
        std::string ext_lower(ext);
        std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), ::tolower);

        if (!ext_lower.empty() && ext_lower[0] != '.') {
            ext_lower = "." + ext_lower;
        }

        const auto &map = get_extension_mime_map();
        auto it = map.find(ext_lower);
        if (it != map.end()) {
            return it->second;
        }
        return "application/octet-stream";
    }

    std::string extension_from_mime_type(std::string_view mime) noexcept {
        std::string mime_lower(mime);
        std::transform(mime_lower.begin(), mime_lower.end(), mime_lower.begin(), ::tolower);

        const auto &map = get_mime_extension_map();
        auto it = map.find(mime_lower);
        if (it != map.end()) {
            return it->second;
        }
        return "";
    }

    result<std::string> detect_mime_type(const path &p) noexcept {
        std::error_code ec;
        if (!std::filesystem::exists(p, ec) || ec) {
            if (ec) {
                return std::unexpected(ec);
            }
            return std::unexpected(make_error(std::errc::no_such_file_or_directory));
        }

        auto info = identify_file_type(p);
        if (info && !info->mime_type.empty()) {
            return info->mime_type;
        }

        return mime_type_from_extension(p.extension().string());
    }

    result<std::string> mime_type_from_content(const path &p) noexcept {
        std::error_code ec;
        if (!std::filesystem::exists(p, ec) || ec) {
            if (ec) {
                return std::unexpected(ec);
            }
            return std::unexpected(make_error(std::errc::no_such_file_or_directory));
        }

        auto info = identify_file_type(p);
        if (info && !info->mime_type.empty()) {
            return info->mime_type;
        }

        return "application/octet-stream";
    }

    std::string mime_type_from_content(std::span<const std::uint8_t> data) noexcept {
        auto info = identify_file_type(data);
        if (!info.mime_type.empty()) {
            return info.mime_type;
        }
        return "application/octet-stream";
    }

    // Magic number signatures
    namespace {
        struct magic_signature {
            std::vector<std::uint8_t> bytes;
            std::size_t offset;
            file_category category;
            std::string mime_type;
            std::string description;
            std::string extension;
        };

        const std::vector<magic_signature> &get_magic_signatures() {
            static const std::vector<magic_signature> signatures = {
                // Images
                {{0xFF, 0xD8, 0xFF}, 0, file_category::image, "image/jpeg", "JPEG image", ".jpg"},
                {{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A},
                 0,
                 file_category::image,
                 "image/png",
                 "PNG image",
                 ".png"},
                {{0x47, 0x49, 0x46, 0x38}, 0, file_category::image, "image/gif", "GIF image", ".gif"},
                {{0x42, 0x4D}, 0, file_category::image, "image/bmp", "BMP image", ".bmp"},
                {{0x52, 0x49, 0x46, 0x46}, 0, file_category::image, "image/webp", "WebP image", ".webp"},

                // Audio
                {{0x49, 0x44, 0x33}, 0, file_category::audio, "audio/mpeg", "MP3 audio", ".mp3"},
                {{0xFF, 0xFB}, 0, file_category::audio, "audio/mpeg", "MP3 audio", ".mp3"},
                {{0xFF, 0xFA}, 0, file_category::audio, "audio/mpeg", "MP3 audio", ".mp3"},
                {{0x52, 0x49, 0x46, 0x46}, 0, file_category::audio, "audio/wav", "WAV audio", ".wav"},
                {{0x4F, 0x67, 0x67, 0x53}, 0, file_category::audio, "audio/ogg", "OGG audio", ".ogg"},
                {{0x66, 0x4C, 0x61, 0x43}, 0, file_category::audio, "audio/flac", "FLAC audio", ".flac"},

                // Video
                {{0x00, 0x00, 0x00}, 0, file_category::video, "video/mp4", "MP4 video", ".mp4"},
                {{0x1A, 0x45, 0xDF, 0xA3}, 0, file_category::video, "video/x-matroska", "Matroska video", ".mkv"},

                // Archives
                {{0x50, 0x4B, 0x03, 0x04}, 0, file_category::archive, "application/zip", "ZIP archive", ".zip"},
                {{0x50, 0x4B, 0x05, 0x06}, 0, file_category::archive, "application/zip", "ZIP archive (empty)", ".zip"},
                {{0x52, 0x61, 0x72, 0x21, 0x1A, 0x07},
                 0,
                 file_category::archive,
                 "application/vnd.rar",
                 "RAR archive",
                 ".rar"},
                {{0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C},
                 0,
                 file_category::archive,
                 "application/x-7z-compressed",
                 "7-Zip archive",
                 ".7z"},
                {{0x1F, 0x8B}, 0, file_category::archive, "application/gzip", "Gzip archive", ".gz"},
                {{0x42, 0x5A, 0x68}, 0, file_category::archive, "application/x-bzip2", "Bzip2 archive", ".bz2"},
                {{0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00},
                 0,
                 file_category::archive,
                 "application/x-xz",
                 "XZ archive",
                 ".xz"},

                // Documents
                {{0x25, 0x50, 0x44, 0x46}, 0, file_category::document, "application/pdf", "PDF document", ".pdf"},
                {{0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1},
                 0,
                 file_category::document,
                 "application/msword",
                 "Microsoft Office document",
                 ".doc"},

                // Executables
                {{0x4D, 0x5A}, 0, file_category::executable, "application/x-msdownload", "Windows executable", ".exe"},
                {{0x7F, 0x45, 0x4C, 0x46},
                 0,
                 file_category::executable,
                 "application/x-executable",
                 "ELF executable",
                 ""},
                {{0xCF, 0xFA, 0xED, 0xFE},
                 0,
                 file_category::executable,
                 "application/x-mach-binary",
                 "Mach-O executable (32-bit)",
                 ""},
                {{0xCE, 0xFA, 0xED, 0xFE},
                 0,
                 file_category::executable,
                 "application/x-mach-binary",
                 "Mach-O executable (64-bit)",
                 ""},

                // Scripts
                {{0x23, 0x21}, 0, file_category::script, "text/x-shellscript", "Shell script", ".sh"},
            };
            return signatures;
        }
    } // namespace

    file_type_info identify_file_type(std::span<const std::uint8_t> data) noexcept {
        file_type_info info;
        info.category = file_category::unknown;
        info.mime_type = "application/octet-stream";
        info.description = "Unknown";

        if (data.empty()) {
            return info;
        }

        const auto &signatures = get_magic_signatures();
        for (const auto &sig : signatures) {
            if (data.size() >= sig.offset + sig.bytes.size()) {
                bool match = true;
                for (std::size_t i = 0; i < sig.bytes.size(); ++i) {
                    if (data[sig.offset + i] != sig.bytes[i]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    info.category = sig.category;
                    info.mime_type = sig.mime_type;
                    info.description = sig.description;
                    info.extension = sig.extension;
                    return info;
                }
            }
        }

        bool is_text = true;
        std::size_t check_size = std::min(data.size(), static_cast<std::size_t>(8192));
        for (std::size_t i = 0; i < check_size; ++i) {
            std::uint8_t c = data[i];
            if (c == 0) {
                is_text = false;
                break;
            }
            if (c < 0x09 || (c > 0x0D && c < 0x20 && c != 0x1B)) {
                is_text = false;
                break;
            }
        }

        if (is_text) {
            info.category = file_category::text;
            info.mime_type = "text/plain";
            info.description = "Text file";
            info.extension = ".txt";
        } else {
            info.category = file_category::binary;
            info.mime_type = "application/octet-stream";
            info.description = "Binary file";
        }

        return info;
    }

    result<file_type_info> identify_file_type(const path &p) noexcept {
        file_type_info info;
        info.category = file_category::unknown;
        info.mime_type = "application/octet-stream";
        info.description = "Unknown";

        std::ifstream file(p, std::ios::binary);
        if (!file) {
            return std::unexpected(make_error(std::errc::no_such_file_or_directory));
        }

        std::vector<std::uint8_t> buffer(8192);
        file.read(reinterpret_cast<char *>(buffer.data()), buffer.size());
        auto bytes_read = file.gcount();
        buffer.resize(static_cast<std::size_t>(bytes_read));

        info = identify_file_type(buffer);

        if (info.extension.empty()) {
            info.extension = p.extension().string();
        }

        return info;
    }

    result<bool> is_text_file(const path &p) noexcept {
        auto info = identify_file_type(p);
        if (!info) {
            return std::unexpected(info.error());
        }
        return info->category == file_category::text || info->category == file_category::script;
    }

    result<bool> is_binary_file(const path &p) noexcept {
        auto info = identify_file_type(p);
        if (!info) {
            return std::unexpected(info.error());
        }
        return info->category != file_category::text && info->category != file_category::script;
    }

    std::string_view file_category_name(file_category cat) noexcept {
        switch (cat) {
        case file_category::unknown:
            return "unknown";
        case file_category::text:
            return "text";
        case file_category::binary:
            return "binary";
        case file_category::image:
            return "image";
        case file_category::audio:
            return "audio";
        case file_category::video:
            return "video";
        case file_category::archive:
            return "archive";
        case file_category::document:
            return "document";
        case file_category::executable:
            return "executable";
        case file_category::script:
            return "script";
        case file_category::data:
            return "data";
        default:
            return "unknown";
        }
    }

    // 5.5 Special file info
    namespace detail {
        result<std::uint64_t> device_id_impl(const path &p) noexcept;
        result<std::uint64_t> inode_impl(const path &p) noexcept;
        result<std::uint64_t> file_index_impl(const path &p) noexcept;
    } // namespace detail

    result<std::uint64_t> device_id(const path &p) noexcept {
        return detail::device_id_impl(p);
    }

    result<std::uint64_t> inode(const path &p) noexcept {
        return detail::inode_impl(p);
    }

    result<std::uint64_t> file_index(const path &p) noexcept {
        return detail::file_index_impl(p);
    }

} // namespace frappe
