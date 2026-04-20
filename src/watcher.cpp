#include "watcher_impl.hpp"

namespace frappe {

// ============================================================================
// watch_event_type_name
// ============================================================================

std::string_view watch_event_type_name(watch_event_type type) noexcept {
    switch (type) {
    case watch_event_type::added:
        return "added";
    case watch_event_type::modified:
        return "modified";
    case watch_event_type::removed:
        return "removed";
    case watch_event_type::renamed:
        return "renamed";
    case watch_event_type::attributes:
        return "attributes";
    case watch_event_type::error:
        return "error";
    default:
        return "unknown";
    }
}

void file_watcher::impl::dispatch_event(const watch_event &event) {
    if (_filter && !_filter(event.file_path)) {
        return;
    }

    if (should_ignore(event.file_path)) {
        return;
    }

    if (_on_event) _on_event(event);

    switch (event.type) {
    case watch_event_type::added:
        if (_on_add) _on_add(event);
        break;
    case watch_event_type::modified:
        if (_on_change) _on_change(event);
        break;
    case watch_event_type::removed:
        if (_on_remove) _on_remove(event);
        break;
    case watch_event_type::renamed:
        if (_on_rename) _on_rename(event);
        break;
    case watch_event_type::error:
        if (_on_error) _on_error(event.error);
        break;
    default:
        break;
    }
}

bool file_watcher::impl::should_ignore(const path &p) const {
    std::string filename = p.filename().string();
    for (const auto &pattern : _options.ignored_patterns) {
        // Simple glob matching (just * for now)
        if (pattern == "*") return true;
        if (pattern.front() == '*' && filename.ends_with(pattern.substr(1))) return true;
        if (pattern.back() == '*' && filename.starts_with(pattern.substr(0, pattern.size() - 1))) return true;
        if (filename == pattern) return true;
    }
    return false;
}

// ============================================================================
// file_watcher public interface
// ============================================================================

file_watcher::file_watcher(watch_options options) : _impl(std::make_unique<impl>(std::move(options))) {}

file_watcher::~file_watcher() = default;

file_watcher::file_watcher(file_watcher &&other) noexcept = default;
file_watcher &file_watcher::operator=(file_watcher &&other) noexcept = default;

void_result file_watcher::add(const path &p) {
    return _impl->add(p);
}

void_result file_watcher::add(std::span<const path> paths) {
    for (const auto &p : paths) {
        auto result = _impl->add(p);
        if (!result) return result;
    }
    return {};
}

void_result file_watcher::remove(const path &p) {
    return _impl->remove(p);
}

void file_watcher::remove_all() {
    _impl->remove_all();
}

void file_watcher::on_event(event_callback callback) {
    _impl->_on_event = std::move(callback);
}

void file_watcher::on_add(event_callback callback) {
    _impl->_on_add = std::move(callback);
}

void file_watcher::on_change(event_callback callback) {
    _impl->_on_change = std::move(callback);
}

void file_watcher::on_remove(event_callback callback) {
    _impl->_on_remove = std::move(callback);
}

void file_watcher::on_rename(event_callback callback) {
    _impl->_on_rename = std::move(callback);
}

void file_watcher::on_error(error_callback callback) {
    _impl->_on_error = std::move(callback);
}

void file_watcher::set_filter(filter_callback filter) {
    _impl->_filter = std::move(filter);
}

void_result file_watcher::start() {
    return _impl->start();
}

void file_watcher::stop() {
    _impl->stop();
}

bool file_watcher::is_running() const noexcept {
    return _impl->_running;
}

std::vector<path> file_watcher::watched_paths() const {
    std::lock_guard<std::mutex> lock(_impl->_mutex);
    return _impl->_watched_paths;
}

const watch_options &file_watcher::options() const noexcept {
    return _impl->_options;
}

// ============================================================================
// Convenience functions
// ============================================================================

result<watch_event> wait_for_change(const path &p, std::chrono::milliseconds timeout) noexcept {
    try {
        std::optional<watch_event> result_event;
        std::mutex mtx;
        std::condition_variable cv;
        bool done = false;

        file_watcher watcher;
        watcher.on_event([&](const watch_event &e) {
            std::lock_guard<std::mutex> lock(mtx);
            result_event = e;
            done = true;
            cv.notify_one();
        });

        auto add_result = watcher.add(p);
        if (!add_result) {
            return std::unexpected(add_result.error());
        }

        auto start_result = watcher.start();
        if (!start_result) {
            return std::unexpected(start_result.error());
        }

        std::unique_lock<std::mutex> lock(mtx);
        if (timeout.count() > 0) {
            cv.wait_for(lock, timeout, [&] { return done; });
        } else {
            cv.wait(lock, [&] { return done; });
        }

        watcher.stop();

        if (result_event) {
            return *result_event;
        }

        return std::unexpected(make_error(std::errc::timed_out));
    } catch (...) {
        return std::unexpected(make_error(std::errc::resource_unavailable_try_again));
    }
}

bool is_watch_supported(const path &p) noexcept {
    std::error_code ec;
    if (!std::filesystem::exists(p, ec)) {
        return false;
    }
    // Most local filesystems support watching
    // Network filesystems may not
    return true;
}

} // namespace frappe
