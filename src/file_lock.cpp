#include "file_lock_impl.hpp"

namespace frappe {

file_lock::impl::~impl() {
    if (_owns_lock) {
        unlock();
    }
}

file_lock::file_lock(const path &p, lock_options options) : _impl(std::make_unique<impl>(p, options)) {}

file_lock::~file_lock() = default;

file_lock::file_lock(file_lock &&other) noexcept = default;
file_lock &file_lock::operator=(file_lock &&other) noexcept = default;

void_result file_lock::lock() {
    return _impl->lock();
}

void_result file_lock::try_lock() {
    return _impl->try_lock();
}

void_result file_lock::try_lock_for(std::chrono::milliseconds timeout) {
    return _impl->try_lock_for(timeout);
}

void file_lock::unlock() {
    _impl->unlock();
}

bool file_lock::owns_lock() const noexcept {
    return _impl->_owns_lock;
}

file_lock::operator bool() const noexcept {
    return owns_lock();
}

const path &file_lock::file_path() const noexcept {
    return _impl->_file_path;
}

lock_type file_lock::type() const noexcept {
    return _impl->_options.type;
}

// ============================================================================
// scoped_file_lock implementation
// ============================================================================

scoped_file_lock::scoped_file_lock(const path &p, lock_type type) : _lock(p, lock_options{.type = type}) {
    auto result = _lock.lock();
    _locked = result.has_value();
}

scoped_file_lock::~scoped_file_lock() {
    if (_locked) {
        _lock.unlock();
    }
}

bool scoped_file_lock::owns_lock() const noexcept {
    return _locked;
}

scoped_file_lock::operator bool() const noexcept {
    return owns_lock();
}

// ============================================================================
// Convenience functions
// ============================================================================

result<file_lock> lock_file(const path &p, lock_options options) noexcept {
    try {
        file_lock lock(p, options);
        auto result = lock.lock();
        if (!result) {
            return std::unexpected(result.error());
        }
        return lock;
    } catch (...) {
        return std::unexpected(make_error(std::errc::resource_unavailable_try_again));
    }
}

result<bool> try_lock_file(const path &p, lock_type type) noexcept {
    try {
        file_lock lock(p, lock_options{.type = type, .mode = lock_mode::non_blocking});
        auto result = lock.try_lock();
        if (!result) {
            if (result.error().value() == static_cast<int>(std::errc::resource_unavailable_try_again)) {
                return false;
            }
            return std::unexpected(result.error());
        }
        return true;
    } catch (...) {
        return std::unexpected(make_error(std::errc::resource_unavailable_try_again));
    }
}

result<bool> is_file_locked(const path &p) noexcept {
    auto result = try_lock_file(p, lock_type::exclusive);
    if (!result) {
        return std::unexpected(result.error());
    }
    return !(*result);
}

std::string_view lock_type_name(lock_type type) noexcept {
    switch (type) {
    case lock_type::shared:
        return "shared";
    case lock_type::exclusive:
        return "exclusive";
    default:
        return "unknown";
    }
}

} // namespace frappe
