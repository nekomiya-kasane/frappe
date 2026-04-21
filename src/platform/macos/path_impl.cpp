#ifdef FRAPPE_PLATFORM_MACOS

#include "frappe/path.hpp"

#include <array>
#include <climits>
#include <cstdlib>
#include <mach-o/dyld.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

namespace frappe::detail {

    result<path> home_path_impl() noexcept {
        const char *home = std::getenv("HOME");
        if (home && *home) {
            return path(home);
        }

        struct passwd *pw = getpwuid(getuid());
        if (pw && pw->pw_dir) {
            return path(pw->pw_dir);
        }

        return std::unexpected(make_error(std::errc::no_such_file_or_directory));
    }

    result<path> executable_path_impl() noexcept {
        std::array<char, PATH_MAX> buffer;
        uint32_t size = static_cast<uint32_t>(buffer.size());

        if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
            char resolved[PATH_MAX];
            if (realpath(buffer.data(), resolved)) {
                return path(resolved);
            }
            return path(buffer.data());
        }

        std::vector<char> large_buffer(size);
        if (_NSGetExecutablePath(large_buffer.data(), &size) == 0) {
            char resolved[PATH_MAX];
            if (realpath(large_buffer.data(), resolved)) {
                return path(resolved);
            }
            return path(large_buffer.data());
        }

        return std::unexpected(make_error(std::errc::no_such_file_or_directory));
    }

    result<path> app_data_path_impl() noexcept {
        auto home = home_path_impl();
        if (!home) {
            return std::unexpected(home.error());
        }

        return *home / "Library" / "Application Support";
    }

    result<path> desktop_path_impl() noexcept {
        auto home = home_path_impl();
        if (!home) {
            return std::unexpected(home.error());
        }

        return *home / "Desktop";
    }

    result<path> documents_path_impl() noexcept {
        auto home = home_path_impl();
        if (!home) {
            return std::unexpected(home.error());
        }

        return *home / "Documents";
    }

    result<path> downloads_path_impl() noexcept {
        auto home = home_path_impl();
        if (!home) {
            return std::unexpected(home.error());
        }

        return *home / "Downloads";
    }

} // namespace frappe::detail

#endif // FRAPPE_PLATFORM_MACOS
