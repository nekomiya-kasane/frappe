#include "watcher_impl.hpp"

#ifdef FRAPPE_PLATFORM_LINUX

#include <cstring>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <unistd.h>

namespace frappe {

void_result file_watcher::impl::add(const path &p) {
    std::lock_guard<std::mutex> lock(_mutex);

    std::error_code ec;
    if (!std::filesystem::exists(p, ec)) {
        return std::unexpected(make_error(std::errc::no_such_file_or_directory));
    }

    _watched_paths.push_back(p);

    // If already running, add watch immediately
    if (_running && _inotify_fd >= 0) {
        uint32_t mask =
            IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB | IN_DELETE_SELF | IN_MOVE_SELF;

        int wd = inotify_add_watch(_inotify_fd, p.c_str(), mask);
        if (wd >= 0) {
            _wd_to_path[wd] = p;
        }

        // Add subdirectories if recursive
        if (_options.recursive && std::filesystem::is_directory(p, ec)) {
            for (const auto &entry : std::filesystem::recursive_directory_iterator(p, ec)) {
                if (entry.is_directory()) {
                    int sub_wd = inotify_add_watch(_inotify_fd, entry.path().c_str(), mask);
                    if (sub_wd >= 0) {
                        _wd_to_path[sub_wd] = entry.path();
                    }
                }
            }
        }
    }

    return {};
}

void_result file_watcher::impl::remove(const path &p) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = std::find(_watched_paths.begin(), _watched_paths.end(), p);
    if (it != _watched_paths.end()) {
        _watched_paths.erase(it);
    }

    // Remove from inotify if running
    if (_running && _inotify_fd >= 0) {
        for (auto it = _wd_to_path.begin(); it != _wd_to_path.end();) {
            if (it->second == p || it->second.string().starts_with(p.string() + "/")) {
                inotify_rm_watch(_inotify_fd, it->first);
                it = _wd_to_path.erase(it);
            } else {
                ++it;
            }
        }
    }

    return {};
}

void file_watcher::impl::remove_all() {
    std::lock_guard<std::mutex> lock(_mutex);
    _watched_paths.clear();

    if (_running && _inotify_fd >= 0) {
        for (const auto &[wd, _] : _wd_to_path) {
            inotify_rm_watch(_inotify_fd, wd);
        }
        _wd_to_path.clear();
    }
}

void_result file_watcher::impl::start() {
    if (_running) {
        return {};
    }

    if (_watched_paths.empty()) {
        return std::unexpected(make_error(std::errc::invalid_argument));
    }

    // Create inotify instance
    _inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (_inotify_fd < 0) {
        return std::unexpected(make_error(errno, std::system_category()));
    }

    // Create stop pipe
    if (pipe(_stop_pipe) < 0) {
        close(_inotify_fd);
        _inotify_fd = -1;
        return std::unexpected(make_error(errno, std::system_category()));
    }

    // Add watches for all paths
    uint32_t mask =
        IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB | IN_DELETE_SELF | IN_MOVE_SELF;

    std::error_code ec;
    for (const auto &p : _watched_paths) {
        int wd = inotify_add_watch(_inotify_fd, p.c_str(), mask);
        if (wd >= 0) {
            _wd_to_path[wd] = p;
        }

        // Add subdirectories if recursive
        if (_options.recursive && std::filesystem::is_directory(p, ec)) {
            for (const auto &entry : std::filesystem::recursive_directory_iterator(p, ec)) {
                if (entry.is_directory()) {
                    int sub_wd = inotify_add_watch(_inotify_fd, entry.path().c_str(), mask);
                    if (sub_wd >= 0) {
                        _wd_to_path[sub_wd] = entry.path();
                    }
                }
            }
        }
    }

    _running = true;
    _watch_thread = std::thread([this]() { watch_loop(); });

    return {};
}

void file_watcher::impl::stop() {
    if (!_running) {
        return;
    }

    _running = false;

    // Signal stop
    if (_stop_pipe[1] >= 0) {
        char c = 'x';
        write(_stop_pipe[1], &c, 1);
    }

    if (_watch_thread.joinable()) {
        _watch_thread.join();
    }

    // Cleanup
    for (const auto &[wd, _] : _wd_to_path) {
        inotify_rm_watch(_inotify_fd, wd);
    }
    _wd_to_path.clear();

    if (_inotify_fd >= 0) {
        close(_inotify_fd);
        _inotify_fd = -1;
    }

    if (_stop_pipe[0] >= 0) {
        close(_stop_pipe[0]);
        _stop_pipe[0] = -1;
    }
    if (_stop_pipe[1] >= 0) {
        close(_stop_pipe[1]);
        _stop_pipe[1] = -1;
    }
}

void file_watcher::impl::watch_loop() {
    constexpr size_t BUF_SIZE = 4096;
    char buffer[BUF_SIZE] ___attribute_((aligned(___alignof_(struct inotify_event))));

    while (_running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(_inotify_fd, &read_fds);
        FD_SET(_stop_pipe[0], &read_fds);

        int max_fd = std::max(_inotify_fd, _stop_pipe[0]);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = _options.poll_interval.count() * 1000;

        int ret = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);

        if (!_running) break;

        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (ret == 0) continue; // Timeout

        if (FD_ISSET(_stop_pipe[0], &read_fds)) {
            break; // Stop signal
        }

        if (FD_ISSET(_inotify_fd, &read_fds)) {
            ssize_t len = read(_inotify_fd, buffer, BUF_SIZE);
            if (len <= 0) continue;

            process_inotify_events(buffer, len);
        }
    }
}

void file_watcher::impl::process_inotify_events(const char *buffer, ssize_t len) {
    const char *ptr = buffer;
    while (ptr < buffer + len) {
        const auto *event = reinterpret_cast<const struct inotify_event *>(ptr);
        process_single_event(event);
        ptr += sizeof(struct inotify_event) + event->len;
    }
}

void file_watcher::impl::process_single_event(const struct inotify_event *event) {
    path dir_path;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _wd_to_path.find(event->wd);
        if (it != _wd_to_path.end()) {
            dir_path = it->second;
        }
    }

    if (dir_path.empty()) return;

    path full_path = dir_path;
    if (event->len > 0) {
        full_path /= event->name;
    }

    watch_event we;
    we.file_path = full_path;
    we.timestamp = std::filesystem::file_time_type::clock::now();
    we.is_directory = (event->mask & IN_ISDIR) != 0;
    we.type = mask_to_event_type(event->mask);

    // Add watch for new directories if recursive
    if (we.type == watch_event_type::added && we.is_directory && _options.recursive) {
        add_recursive_watch(full_path);
    }

    dispatch_event(we);
}

watch_event_type file_watcher::impl::mask_to_event_type(uint32_t mask) {
    if (mask & IN_CREATE) return watch_event_type::added;
    if (mask & IN_DELETE) return watch_event_type::removed;
    if (mask & IN_MODIFY) return watch_event_type::modified;
    if (mask & (IN_MOVED_FROM | IN_MOVED_TO)) return watch_event_type::renamed;
    if (mask & IN_ATTRIB) return watch_event_type::attributes;
    if (mask & (IN_DELETE_SELF | IN_MOVE_SELF)) return watch_event_type::removed;
    return watch_event_type::modified;
}

void file_watcher::impl::add_recursive_watch(const path &dir_path) {
    uint32_t mask =
        IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB | IN_DELETE_SELF | IN_MOVE_SELF;
    int wd = inotify_add_watch(_inotify_fd, dir_path.c_str(), mask);
    if (wd >= 0) {
        std::lock_guard<std::mutex> lock(_mutex);
        _wd_to_path[wd] = dir_path;
    }
}

} // namespace frappe

#endif // FRAPPE_PLATFORM_LINUX
