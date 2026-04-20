#include "file_lock_impl.hpp"

#ifdef FRAPPE_PLATFORM_MACOS

#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <thread>
#include <unistd.h>

namespace frappe {

void_result file_lock::impl::lock() {
    if (_owns_lock) {
        return {};
    }

    // Open file
    int flags = (_options.type == lock_type::shared) ? O_RDONLY : O_RDWR;
    _fd = open(_file_path.c_str(), flags);

    if (_fd < 0) {
        return std::unexpected(make_error(errno, std::system_category()));
    }

    // Lock file using flock
    int lock_op = (_options.type == lock_type::exclusive) ? LOCK_EX : LOCK_SH;
    if (_options.mode == lock_mode::non_blocking) {
        lock_op |= LOCK_NB;
    }

    if (flock(_fd, lock_op) != 0) {
        int err = errno;
        close(_fd);
        _fd = -1;

        if (err == EWOULDBLOCK) {
            return std::unexpected(make_error(std::errc::resource_unavailable_try_again));
        }
        return std::unexpected(make_error(err, std::system_category()));
    }

    _owns_lock = true;
    return {};
}

void_result file_lock::impl::try_lock() {
    auto saved_mode = _options.mode;
    _options.mode = lock_mode::non_blocking;
    auto result = lock();
    _options.mode = saved_mode;
    return result;
}

void_result file_lock::impl::try_lock_for(std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();

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

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void file_lock::impl::unlock() {
    if (!_owns_lock || _fd < 0) {
        return;
    }

    flock(_fd, LOCK_UN);
    close(_fd);
    _fd = -1;
    _owns_lock = false;
}

} // namespace frappe

#endif // FRAPPE_PLATFORM_MACOS
