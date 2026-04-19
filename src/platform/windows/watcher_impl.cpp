#include "watcher_impl.hpp"

#ifdef FRAPPE_PLATFORM_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace frappe {

void_result file_watcher::impl::add(const path& p) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    std::error_code ec;
    if (!std::filesystem::exists(p, ec)) {
        return std::unexpected(make_error(std::errc::no_such_file_or_directory));
    }
    
    _watched_paths.push_back(p);
    return {};
}

void_result file_watcher::impl::remove(const path& p) {
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

void_result file_watcher::impl::start() {
    if (_running) {
        return {};
    }
    
    if (_watched_paths.empty()) {
        return std::unexpected(make_error(std::errc::invalid_argument));
    }
    
    _stop_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!_stop_event) {
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
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
    
    if (_stop_event) {
        SetEvent(static_cast<HANDLE>(_stop_event));
    }
    
    if (_watch_thread.joinable()) {
        _watch_thread.join();
    }
    
    if (_stop_event) {
        CloseHandle(static_cast<HANDLE>(_stop_event));
        _stop_event = nullptr;
    }
    
    for (auto& handle : _dir_handles) {
        if (handle && handle != INVALID_HANDLE_VALUE) {
            CloseHandle(static_cast<HANDLE>(handle));
        }
    }
    _dir_handles.clear();
}

void file_watcher::impl::watch_loop() {
    std::vector<HANDLE> handles;
    std::vector<path> handle_paths;
    std::vector<std::vector<BYTE>> buffers;
    std::vector<OVERLAPPED> overlappeds;
    
    // Open directory handles
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (const auto& p : _watched_paths) {
            HANDLE hDir = CreateFileW(
                p.c_str(),
                FILE_LIST_DIRECTORY,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                nullptr,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                nullptr
            );
            
            if (hDir != INVALID_HANDLE_VALUE) {
                _dir_handles.push_back(hDir);
                handles.push_back(hDir);
                handle_paths.push_back(p);
                buffers.emplace_back(64 * 1024);
                overlappeds.emplace_back();
            }
        }
    }
    
    if (handles.empty()) {
        return;
    }
    
    auto process_notify_info = [this](FILE_NOTIFY_INFORMATION* info, const path& base_path) {
        while (true) {
            std::wstring filename(info->FileName, info->FileNameLength / sizeof(WCHAR));
            path full_path = base_path / filename;
            
            watch_event event;
            event.file_path = full_path;
            event.timestamp = std::filesystem::file_time_type::clock::now();
            
            switch (info->Action) {
                case FILE_ACTION_ADDED:    event.type = watch_event_type::added; break;
                case FILE_ACTION_REMOVED:  event.type = watch_event_type::removed; break;
                case FILE_ACTION_MODIFIED: event.type = watch_event_type::modified; break;
                case FILE_ACTION_RENAMED_OLD_NAME:
                    event.type = watch_event_type::renamed;
                    event.old_path = full_path;
                    break;
                case FILE_ACTION_RENAMED_NEW_NAME:
                    event.type = watch_event_type::renamed;
                    break;
                default:
                    event.type = watch_event_type::attributes;
                    break;
            }
            
            std::error_code ec;
            event.is_directory = std::filesystem::is_directory(full_path, ec);
            dispatch_event(event);
            
            if (info->NextEntryOffset == 0) break;
            info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                reinterpret_cast<BYTE*>(info) + info->NextEntryOffset);
        }
    };
    
    // Create events for overlapped IO
    std::vector<HANDLE> events;
    for (size_t i = 0; i < handles.size(); ++i) {
        HANDLE hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        events.push_back(hEvent);
        overlappeds[i].hEvent = hEvent;
    }
    
    // Start watching
    DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                   FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
                   FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION;
    
    for (size_t i = 0; i < handles.size(); ++i) {
        ReadDirectoryChangesW(
            handles[i],
            buffers[i].data(),
            static_cast<DWORD>(buffers[i].size()),
            _options.recursive ? TRUE : FALSE,
            filter,
            nullptr,
            &overlappeds[i],
            nullptr
        );
    }
    
    // Add stop event
    std::vector<HANDLE> wait_handles = events;
    wait_handles.push_back(static_cast<HANDLE>(_stop_event));
    
    while (_running) {
        DWORD result = WaitForMultipleObjects(
            static_cast<DWORD>(wait_handles.size()),
            wait_handles.data(),
            FALSE,
            static_cast<DWORD>(_options.poll_interval.count())
        );
        
        if (!_running) break;
        
        if (result == WAIT_TIMEOUT) {
            continue;
        }
        
        if (result < WAIT_OBJECT_0 || result >= WAIT_OBJECT_0 + events.size()) continue;
        
        size_t idx = result - WAIT_OBJECT_0;
        
        DWORD bytes_returned = 0;
        if (GetOverlappedResult(handles[idx], &overlappeds[idx], &bytes_returned, FALSE) && bytes_returned > 0) {
            process_notify_info(reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffers[idx].data()), handle_paths[idx]);
        }
        
        // Restart watching
        ResetEvent(events[idx]);
        ReadDirectoryChangesW(handles[idx], buffers[idx].data(), static_cast<DWORD>(buffers[idx].size()),
                              _options.recursive ? TRUE : FALSE, filter, nullptr, &overlappeds[idx], nullptr);
    }
    
    // Cleanup events
    for (auto& hEvent : events) {
        CloseHandle(hEvent);
    }
}

} // namespace frappe

#endif // FRAPPE_PLATFORM_WINDOWS
