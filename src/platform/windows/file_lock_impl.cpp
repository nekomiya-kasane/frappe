#include "file_lock_impl.hpp"

#ifdef FRAPPE_PLATFORM_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace frappe {

    void_result file_lock::impl::lock() {
        if (_owns_lock) {
            return {};
        }

        // Open file
        const DWORD access = (_options.type == lock_type::shared) ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE);
        const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

        _handle =
            CreateFileW(_file_path.c_str(), access, share, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

        if (_handle == INVALID_HANDLE_VALUE) {
            return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
        }

        // Lock file
        DWORD flags = (_options.type == lock_type::exclusive) ? LOCKFILE_EXCLUSIVE_LOCK : 0;
        if (_options.mode == lock_mode::non_blocking) {
            flags |= LOCKFILE_FAIL_IMMEDIATELY;
        }

        OVERLAPPED overlapped = {};
        overlapped.Offset = static_cast<DWORD>(_options.offset & 0xFFFFFFFF);
        overlapped.OffsetHigh = static_cast<DWORD>((_options.offset >> 32) & 0xFFFFFFFF);

        const DWORD length_low = (_options.length == 0) ? 0xFFFFFFFF : static_cast<DWORD>(_options.length & 0xFFFFFFFF);
        const DWORD length_high =
            (_options.length == 0) ? 0x7FFFFFFF : static_cast<DWORD>((_options.length >> 32) & 0xFFFFFFFF);

        if (!LockFileEx(_handle, flags, 0, length_low, length_high, &overlapped)) {
            const DWORD error = GetLastError();
            CloseHandle(_handle);
            _handle = INVALID_HANDLE_VALUE;

            if (error == ERROR_LOCK_VIOLATION) {
                return std::unexpected(make_error(std::errc::resource_unavailable_try_again));
            }
            return std::unexpected(make_error(static_cast<int>(error), std::system_category()));
        }

        _owns_lock = true;
        return {};
    }

    void_result file_lock::impl::try_lock() {
        const auto saved_mode = _options.mode;
        _options.mode = lock_mode::non_blocking;
        auto result = lock();
        _options.mode = saved_mode;
        return result;
    }

    void_result file_lock::impl::try_lock_for(std::chrono::milliseconds timeout) {
        const auto start = std::chrono::steady_clock::now();

        while (true) {
            auto result = try_lock();
            if (result) {
                return result;
            }

            if (result.error().value() != static_cast<int>(std::errc::resource_unavailable_try_again)) {
                return result;
            }

            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed >= timeout) {
                return std::unexpected(make_error(std::errc::timed_out));
            }

            Sleep(10);
        }
    }

    void file_lock::impl::unlock() {
        if (!_owns_lock || _handle == INVALID_HANDLE_VALUE) {
            return;
        }

        OVERLAPPED overlapped = {};
        overlapped.Offset = static_cast<DWORD>(_options.offset & 0xFFFFFFFF);
        overlapped.OffsetHigh = static_cast<DWORD>((_options.offset >> 32) & 0xFFFFFFFF);

        const DWORD length_low = (_options.length == 0) ? 0xFFFFFFFF : static_cast<DWORD>(_options.length & 0xFFFFFFFF);
        const DWORD length_high =
            (_options.length == 0) ? 0x7FFFFFFF : static_cast<DWORD>((_options.length >> 32) & 0xFFFFFFFF);

        UnlockFileEx(_handle, 0, length_low, length_high, &overlapped);
        CloseHandle(_handle);
        _handle = INVALID_HANDLE_VALUE;
        _owns_lock = false;
    }

} // namespace frappe

#endif // FRAPPE_PLATFORM_WINDOWS
