#ifdef FRAPPE_PLATFORM_WINDOWS

#define NOMINMAX
#include "frappe/status.hpp"

#include <Windows.h>

namespace frappe::detail {

    result<file_time_type> creation_time_impl(const path &p) noexcept {
        HANDLE hFile = CreateFileW(p.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

        if (hFile == INVALID_HANDLE_VALUE) {
            return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
        }

        FILETIME creation_time, access_time, write_time;
        if (!GetFileTime(hFile, &creation_time, &access_time, &write_time)) {
            auto err = make_error(static_cast<int>(GetLastError()), std::system_category());
            CloseHandle(hFile);
            return std::unexpected(err);
        }

        CloseHandle(hFile);

        ULARGE_INTEGER uli;
        uli.LowPart = creation_time.dwLowDateTime;
        uli.HighPart = creation_time.dwHighDateTime;

        auto duration = std::chrono::duration_cast<file_time_type::duration>(
            std::chrono::nanoseconds(uli.QuadPart * 100) - std::chrono::nanoseconds(116444736000000000LL));

        return file_time_type(duration);
    }

    result<file_time_type> last_access_time_impl(const path &p) noexcept {
        HANDLE hFile = CreateFileW(p.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

        if (hFile == INVALID_HANDLE_VALUE) {
            return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
        }

        FILETIME creation_time, access_time, write_time;
        if (!GetFileTime(hFile, &creation_time, &access_time, &write_time)) {
            auto err = make_error(static_cast<int>(GetLastError()), std::system_category());
            CloseHandle(hFile);
            return std::unexpected(err);
        }

        CloseHandle(hFile);

        ULARGE_INTEGER uli;
        uli.LowPart = access_time.dwLowDateTime;
        uli.HighPart = access_time.dwHighDateTime;

        auto duration = std::chrono::duration_cast<file_time_type::duration>(
            std::chrono::nanoseconds(uli.QuadPart * 100) - std::chrono::nanoseconds(116444736000000000LL));

        return file_time_type(duration);
    }

    result<file_id> get_file_id_impl(const path &p) noexcept {
        HANDLE hFile = CreateFileW(p.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                   OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

        if (hFile == INVALID_HANDLE_VALUE) {
            return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
        }

        BY_HANDLE_FILE_INFORMATION info;
        if (!GetFileInformationByHandle(hFile, &info)) {
            auto err = make_error(static_cast<int>(GetLastError()), std::system_category());
            CloseHandle(hFile);
            return std::unexpected(err);
        }

        CloseHandle(hFile);

        file_id result;
        result.device = info.dwVolumeSerialNumber;
        result.inode = (static_cast<std::uint64_t>(info.nFileIndexHigh) << 32) | info.nFileIndexLow;

        return result;
    }

    result<std::uintmax_t> max_hard_links_impl(const path &p) noexcept {
        (void)p;
        return std::uintmax_t{1024};
    }

} // namespace frappe::detail

#endif // FRAPPE_PLATFORM_WINDOWS
