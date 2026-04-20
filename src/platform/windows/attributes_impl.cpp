#ifdef FRAPPE_PLATFORM_WINDOWS

#define NOMINMAX
#include "frappe/attributes.hpp"

#include <Windows.h>
#include <string>
#include <vector>

namespace frappe::detail {

result<bool> has_xattr_impl(const path &p, std::string_view name) noexcept {
    std::wstring stream_path = p.wstring() + L":" + std::wstring(name.begin(), name.end());

    HANDLE hFile = CreateFileW(stream_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND) {
            return false;
        }
        return std::unexpected(make_error(static_cast<int>(error), std::system_category()));
    }

    CloseHandle(hFile);
    return true;
}

result<std::vector<std::uint8_t>> get_xattr_impl(const path &p, std::string_view name) noexcept {
    std::wstring stream_path = p.wstring() + L":" + std::wstring(name.begin(), name.end());

    HANDLE hFile = CreateFileW(stream_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(hFile, &size)) {
        auto err = make_error(static_cast<int>(GetLastError()), std::system_category());
        CloseHandle(hFile);
        return std::unexpected(err);
    }

    std::vector<std::uint8_t> result;
    result.resize(static_cast<std::size_t>(size.QuadPart));

    DWORD bytes_read = 0;
    if (!ReadFile(hFile, result.data(), static_cast<DWORD>(result.size()), &bytes_read, nullptr)) {
        auto err = make_error(static_cast<int>(GetLastError()), std::system_category());
        CloseHandle(hFile);
        return std::unexpected(err);
    }

    result.resize(bytes_read);
    CloseHandle(hFile);
    return result;
}

result<std::vector<std::string>> list_xattr_impl(const path &p) noexcept {
    std::vector<std::string> result;

    WIN32_FIND_STREAM_DATA stream_data;
    HANDLE hFind = FindFirstStreamW(p.c_str(), FindStreamInfoStandard, &stream_data, 0);

    if (hFind == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_HANDLE_EOF || error == ERROR_FILE_NOT_FOUND) {
            return result;
        }
        return std::unexpected(make_error(static_cast<int>(error), std::system_category()));
    }

    do {
        std::wstring stream_name = stream_data.cStreamName;
        if (stream_name.size() > 2 && stream_name[0] == L':') {
            auto colon_pos = stream_name.find(L':', 1);
            if (colon_pos != std::wstring::npos) {
                std::wstring name = stream_name.substr(1, colon_pos - 1);
                if (!name.empty() && name != L"$DATA") {
                    result.push_back(std::string(name.begin(), name.end()));
                }
            }
        }
    } while (FindNextStreamW(hFind, &stream_data));

    FindClose(hFind);
    return result;
}

result<std::size_t> size_xattr_impl(const path &p, std::string_view name) noexcept {
    std::wstring stream_path = p.wstring() + L":" + std::wstring(name.begin(), name.end());

    HANDLE hFile = CreateFileW(stream_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(hFile, &size)) {
        auto err = make_error(static_cast<int>(GetLastError()), std::system_category());
        CloseHandle(hFile);
        return std::unexpected(err);
    }

    CloseHandle(hFile);
    return static_cast<std::size_t>(size.QuadPart);
}

void_result set_xattr_impl(const path &p, std::string_view name, std::span<const std::uint8_t> value) noexcept {
    std::wstring stream_path = p.wstring() + L":" + std::wstring(name.begin(), name.end());

    HANDLE hFile = CreateFileW(stream_path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    DWORD bytes_written = 0;
    if (!WriteFile(hFile, value.data(), static_cast<DWORD>(value.size()), &bytes_written, nullptr)) {
        auto err = make_error(static_cast<int>(GetLastError()), std::system_category());
        CloseHandle(hFile);
        return std::unexpected(err);
    }

    CloseHandle(hFile);
    return {};
}

void_result remove_xattr_impl(const path &p, std::string_view name) noexcept {
    std::wstring stream_path = p.wstring() + L":" + std::wstring(name.begin(), name.end());

    if (!DeleteFileW(stream_path.c_str())) {
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }
    return {};
}

result<bool> xattr_supported_impl(const path &p) noexcept {
    wchar_t volume_path[MAX_PATH] = {0};
    if (!GetVolumePathNameW(p.c_str(), volume_path, MAX_PATH)) {
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    wchar_t fs_name[MAX_PATH] = {0};
    DWORD flags = 0;
    if (!GetVolumeInformationW(volume_path, nullptr, 0, nullptr, nullptr, &flags, fs_name, MAX_PATH)) {
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    return (flags & FILE_NAMED_STREAMS) != 0;
}

// 5.5 Special file info
result<std::uint64_t> device_id_impl(const path &p) noexcept {
    HANDLE hFile = CreateFileW(p.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                               OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle(hFile, &info)) {
        CloseHandle(hFile);
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    CloseHandle(hFile);
    return static_cast<std::uint64_t>(info.dwVolumeSerialNumber);
}

result<std::uint64_t> file_index_impl(const path &p) noexcept {
    HANDLE hFile = CreateFileW(p.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                               OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle(hFile, &info)) {
        CloseHandle(hFile);
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    CloseHandle(hFile);
    return (static_cast<std::uint64_t>(info.nFileIndexHigh) << 32) | info.nFileIndexLow;
}

result<std::uint64_t> inode_impl(const path &p) noexcept {
    return file_index_impl(p);
}

} // namespace frappe::detail

namespace frappe {

result<std::vector<std::string>> list_alternate_streams(const path &p) noexcept {
    return detail::list_xattr_impl(p);
}

result<bool> has_alternate_stream(const path &p, std::string_view stream_name) noexcept {
    return detail::has_xattr_impl(p, stream_name);
}

} // namespace frappe

#endif // FRAPPE_PLATFORM_WINDOWS
