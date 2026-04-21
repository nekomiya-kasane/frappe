#ifdef FRAPPE_PLATFORM_LINUX

#include "frappe/attributes.hpp"

#include <cerrno>
#include <cstring>
#include <sys/xattr.h>

namespace frappe::detail {

    result<bool> has_xattr_impl(const path &p, std::string_view name) noexcept {
        std::string attr_name(name);
        ssize_t size = getxattr(p.c_str(), attr_name.c_str(), nullptr, 0);

        if (size < 0) {
            if (errno == ENODATA || errno == ENOATTR) {
                return false;
            }
            return std::unexpected(make_error(errno, std::system_category()));
        }

        return true;
    }

    result<std::vector<std::uint8_t>> get_xattr_impl(const path &p, std::string_view name) noexcept {
        std::string attr_name(name);

        ssize_t size = getxattr(p.c_str(), attr_name.c_str(), nullptr, 0);
        if (size < 0) {
            return std::unexpected(make_error(errno, std::system_category()));
        }

        std::vector<std::uint8_t> result;
        result.resize(static_cast<std::size_t>(size));

        ssize_t actual_size = getxattr(p.c_str(), attr_name.c_str(), result.data(), result.size());
        if (actual_size < 0) {
            return std::unexpected(make_error(errno, std::system_category()));
        }

        result.resize(static_cast<std::size_t>(actual_size));
        return result;
    }

    result<std::vector<std::string>> list_xattr_impl(const path &p) noexcept {
        std::vector<std::string> result;

        ssize_t size = listxattr(p.c_str(), nullptr, 0);
        if (size < 0) {
            if (errno == ENOTSUP) {
                return result;
            }
            return std::unexpected(make_error(errno, std::system_category()));
        }

        if (size == 0) {
            return result;
        }

        std::vector<char> buffer(static_cast<std::size_t>(size));
        ssize_t actual_size = listxattr(p.c_str(), buffer.data(), buffer.size());
        if (actual_size < 0) {
            return std::unexpected(make_error(errno, std::system_category()));
        }

        const char *ptr = buffer.data();
        const char *end = ptr + actual_size;
        while (ptr < end) {
            std::string name(ptr);
            if (!name.empty()) {
                result.push_back(name);
            }
            ptr += name.size() + 1;
        }

        return result;
    }

    result<std::size_t> size_xattr_impl(const path &p, std::string_view name) noexcept {
        std::string attr_name(name);
        ssize_t size = getxattr(p.c_str(), attr_name.c_str(), nullptr, 0);

        if (size < 0) {
            return std::unexpected(make_error(errno, std::system_category()));
        }

        return static_cast<std::size_t>(size);
    }

    void_result set_xattr_impl(const path &p, std::string_view name, std::span<const std::uint8_t> value) noexcept {
        std::string attr_name(name);

        if (setxattr(p.c_str(), attr_name.c_str(), value.data(), value.size(), 0) != 0) {
            return std::unexpected(make_error(errno, std::system_category()));
        }
        return {};
    }

    void_result remove_xattr_impl(const path &p, std::string_view name) noexcept {
        std::string attr_name(name);

        if (removexattr(p.c_str(), attr_name.c_str()) != 0) {
            return std::unexpected(make_error(errno, std::system_category()));
        }
        return {};
    }

    result<bool> xattr_supported_impl(const path &p) noexcept {
        ssize_t size = listxattr(p.c_str(), nullptr, 0);
        if (size < 0) {
            if (errno == ENOTSUP) {
                return false;
            }
            return std::unexpected(make_error(errno, std::system_category()));
        }

        return true;
    }

    // 5.5 Special file info
    result<std::uint64_t> device_id_impl(const path &p) noexcept {
        struct stat st;
        if (stat(p.c_str(), &st) != 0) {
            return std::unexpected(make_error(errno, std::system_category()));
        }
        return static_cast<std::uint64_t>(st.st_dev);
    }

    result<std::uint64_t> inode_impl(const path &p) noexcept {
        struct stat st;
        if (stat(p.c_str(), &st) != 0) {
            return std::unexpected(make_error(errno, std::system_category()));
        }
        return static_cast<std::uint64_t>(st.st_ino);
    }

    result<std::uint64_t> file_index_impl(const path &p) noexcept {
        return inode_impl(p);
    }

} // namespace frappe::detail

#endif // FRAPPE_PLATFORM_LINUX
