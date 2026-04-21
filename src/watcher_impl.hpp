#ifndef FRAPPE_WATCHER_IMPL_HPP
#define FRAPPE_WATCHER_IMPL_HPP

#include "frappe/watcher.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

namespace frappe {

    class file_watcher::impl {
      public:
        watch_options _options;
        std::vector<path> _watched_paths;

        event_callback _on_event;
        event_callback _on_add;
        event_callback _on_change;
        event_callback _on_remove;
        event_callback _on_rename;
        error_callback _on_error;
        filter_callback _filter;

        std::atomic<bool> _running{false};
        std::thread _watch_thread;
        std::mutex _mutex;

        // Platform-specific handle
#ifdef _WIN32
        void *_stop_event = nullptr;
        std::vector<void *> _dir_handles;
#else
        int _inotify_fd = -1;
        int _stop_pipe[2] = {-1, -1};
        std::unordered_map<int, path> _wd_to_path;
#endif

        // Debounce queue
        struct pending_event {
            watch_event event;
            std::chrono::steady_clock::time_point time;
        };
        std::queue<pending_event> _pending_events;

        explicit impl(watch_options opts) : _options(std::move(opts)) {}
        ~impl() { stop(); }

        void_result add(const path &p);
        void_result remove(const path &p);
        void remove_all();
        void_result start();
        void stop();

        void dispatch_event(const watch_event &event);
        bool should_ignore(const path &p) const;

      private:
        void watch_loop();
        void process_pending_events();

#ifndef _WIN32
        void process_inotify_events(const char *buffer, ssize_t len);
        void process_single_event(const struct inotify_event *event);
        watch_event_type mask_to_event_type(uint32_t mask);
        void add_recursive_watch(const path &dir_path);
#endif
    };

} // namespace frappe

#endif // FRAPPE_WATCHER_IMPL_HPP
