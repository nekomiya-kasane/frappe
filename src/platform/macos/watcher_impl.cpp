#include "watcher_impl.hpp"

#ifdef FRAPPE_PLATFORM_MACOS

#include <CoreServices/CoreServices.h>
#include <dispatch/dispatch.h>

namespace frappe {

void_result file_watcher::impl::add(const path &p) {
    std::lock_guard<std::mutex> lock(_mutex);

    std::error_code ec;
    if (!std::filesystem::exists(p, ec)) {
        return std::unexpected(make_error(std::errc::no_such_file_or_directory));
    }

    _watched_paths.push_back(p);
    return {};
}

void_result file_watcher::impl::remove(const path &p) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = std::find(_watched_paths.begin(), _watched_paths.end(), p);
    if (it != _watched_paths.end()) {
        _watched_paths.erase(it);
    }
    return {};
}

void file_watcher::impl::remove_all() {
    std::lock_guard<std::mutex> lock(_mutex);
    _watched_paths.clear();
}

// FSEvents callback
static void fsevents_callback(ConstFSEventStreamRef streamRef, void *clientCallBackInfo, size_t numEvents,
                              void *eventPaths, const FSEventStreamEventFlags eventFlags[],
                              const FSEventStreamEventId eventIds[]) {
    auto *impl = static_cast<file_watcher::impl *>(clientCallBackInfo);
    char **paths = static_cast<char **>(eventPaths);

    for (size_t i = 0; i < numEvents; ++i) {
        watch_event event;
        event.file_path = paths[i];
        event.timestamp = std::filesystem::file_time_type::clock::now();

        FSEventStreamEventFlags flags = eventFlags[i];

        if (flags & kFSEventStreamEventFlagItemCreated) {
            event.type = watch_event_type::added;
        } else if (flags & kFSEventStreamEventFlagItemRemoved) {
            event.type = watch_event_type::removed;
        } else if (flags & kFSEventStreamEventFlagItemModified) {
            event.type = watch_event_type::modified;
        } else if (flags & kFSEventStreamEventFlagItemRenamed) {
            event.type = watch_event_type::renamed;
        } else if (flags & (kFSEventStreamEventFlagItemChangeOwner | kFSEventStreamEventFlagItemXattrMod |
                            kFSEventStreamEventFlagItemInodeMetaMod)) {
            event.type = watch_event_type::attributes;
        } else {
            event.type = watch_event_type::modified;
        }

        event.is_directory = (flags & kFSEventStreamEventFlagItemIsDir) != 0;

        impl->dispatch_event(event);
    }
}

void_result file_watcher::impl::start() {
    if (_running) {
        return {};
    }

    if (_watched_paths.empty()) {
        return std::unexpected(make_error(std::errc::invalid_argument));
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

    if (_watch_thread.joinable()) {
        _watch_thread.join();
    }
}

void file_watcher::impl::watch_loop() {
    // Create CFArray of paths to watch
    std::vector<CFStringRef> cf_paths;
    for (const auto &p : _watched_paths) {
        CFStringRef cf_path = CFStringCreateWithCString(kCFAllocatorDefault, p.c_str(), kCFStringEncodingUTF8);
        cf_paths.push_back(cf_path);
    }

    CFArrayRef paths_to_watch = CFArrayCreate(kCFAllocatorDefault, reinterpret_cast<const void **>(cf_paths.data()),
                                              cf_paths.size(), &kCFTypeArrayCallBacks);

    // Create FSEvents stream
    FSEventStreamContext context = {0, this, nullptr, nullptr, nullptr};

    FSEventStreamCreateFlags flags = kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagNoDefer;

    FSEventStreamRef stream =
        FSEventStreamCreate(kCFAllocatorDefault, &fsevents_callback, &context, paths_to_watch,
                            kFSEventStreamEventIdSinceNow, _options.debounce.count() / 1000.0, flags);

    // Create dispatch queue
    dispatch_queue_t queue = dispatch_queue_create("frappe.watcher", DISPATCH_QUEUE_SERIAL);
    FSEventStreamSetDispatchQueue(stream, queue);

    // Start stream
    FSEventStreamStart(stream);

    // Wait for stop signal
    while (_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Cleanup
    FSEventStreamStop(stream);
    FSEventStreamInvalidate(stream);
    FSEventStreamRelease(stream);
    dispatch_release(queue);

    CFRelease(paths_to_watch);
    for (auto cf_path : cf_paths) {
        CFRelease(cf_path);
    }
}

} // namespace frappe

#endif // FRAPPE_PLATFORM_MACOS
