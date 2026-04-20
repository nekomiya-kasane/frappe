#ifdef FRAPPE_PLATFORM_WINDOWS

#include "frappe/path.hpp"

#include <ShlObj.h>
#include <Windows.h>
#include <array>

namespace frappe::detail {

result<path> home_path_impl() noexcept {
    wchar_t *path_ptr = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &path_ptr);
    if (FAILED(hr)) {
        return std::unexpected(make_error(static_cast<int>(hr), std::system_category()));
    }

    path result(path_ptr);
    CoTaskMemFree(path_ptr);
    return result;
}

result<path> executable_path_impl() noexcept {
    std::array<wchar_t, MAX_PATH> buffer;
    DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));

    if (size == 0) {
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    if (size == buffer.size()) {
        std::vector<wchar_t> large_buffer(32768);
        size = GetModuleFileNameW(nullptr, large_buffer.data(), static_cast<DWORD>(large_buffer.size()));
        if (size == 0 || size == large_buffer.size()) {
            return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
        }
        return path(large_buffer.data());
    }

    return path(buffer.data());
}

result<path> app_data_path_impl() noexcept {
    wchar_t *path_ptr = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path_ptr);
    if (FAILED(hr)) {
        return std::unexpected(make_error(static_cast<int>(hr), std::system_category()));
    }

    path result(path_ptr);
    CoTaskMemFree(path_ptr);
    return result;
}

result<path> desktop_path_impl() noexcept {
    wchar_t *path_ptr = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &path_ptr);
    if (FAILED(hr)) {
        return std::unexpected(make_error(static_cast<int>(hr), std::system_category()));
    }

    path result(path_ptr);
    CoTaskMemFree(path_ptr);
    return result;
}

result<path> documents_path_impl() noexcept {
    wchar_t *path_ptr = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path_ptr);
    if (FAILED(hr)) {
        return std::unexpected(make_error(static_cast<int>(hr), std::system_category()));
    }

    path result(path_ptr);
    CoTaskMemFree(path_ptr);
    return result;
}

result<path> downloads_path_impl() noexcept {
    wchar_t *path_ptr = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Downloads, 0, nullptr, &path_ptr);
    if (FAILED(hr)) {
        return std::unexpected(make_error(static_cast<int>(hr), std::system_category()));
    }

    path result(path_ptr);
    CoTaskMemFree(path_ptr);
    return result;
}

} // namespace frappe::detail

#endif // FRAPPE_PLATFORM_WINDOWS
