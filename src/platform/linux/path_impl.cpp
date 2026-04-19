#ifdef FRAPPE_PLATFORM_LINUX

#include "frappe/path.hpp"
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <climits>
#include <cstdlib>
#include <array>

namespace frappe::detail {

result<path> home_path_impl() noexcept {
    const char* home = std::getenv("HOME");
    if (home && *home) {
        return path(home);
    }
    
    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir) {
        return path(pw->pw_dir);
    }
    
    return std::unexpected(make_error(std::errc::no_such_file_or_directory));
}

result<path> executable_path_impl() noexcept {
    std::array<char, PATH_MAX> buffer;
    ssize_t len = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    
    if (len == -1) {
        return std::unexpected(make_error(errno, std::system_category()));
    }
    
    buffer[len] = '\0';
    return path(buffer.data());
}

result<path> app_data_path_impl() noexcept {
    const char* xdg_data = std::getenv("XDG_DATA_HOME");
    if (xdg_data && *xdg_data) {
        return path(xdg_data);
    }
    
    auto home = home_path_impl();
    if (!home) {
        return std::unexpected(home.error());
    }
    
    return *home / ".local" / "share";
}

result<path> desktop_path_impl() noexcept {
    const char* xdg_desktop = std::getenv("XDG_DESKTOP_DIR");
    if (xdg_desktop && *xdg_desktop) {
        return path(xdg_desktop);
    }
    
    auto home = home_path_impl();
    if (!home) {
        return std::unexpected(home.error());
    }
    
    return *home / "Desktop";
}

result<path> documents_path_impl() noexcept {
    const char* xdg_docs = std::getenv("XDG_DOCUMENTS_DIR");
    if (xdg_docs && *xdg_docs) {
        return path(xdg_docs);
    }
    
    auto home = home_path_impl();
    if (!home) {
        return std::unexpected(home.error());
    }
    
    return *home / "Documents";
}

result<path> downloads_path_impl() noexcept {
    const char* xdg_download = std::getenv("XDG_DOWNLOAD_DIR");
    if (xdg_download && *xdg_download) {
        return path(xdg_download);
    }
    
    auto home = home_path_impl();
    if (!home) {
        return std::unexpected(home.error());
    }
    
    return *home / "Downloads";
}

} // namespace frappe::detail

#endif // FRAPPE_PLATFORM_LINUX
