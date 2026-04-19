# frappe - Cross-Platform Filesystem Library

A modern C++23 cross-platform filesystem library with comprehensive filesystem operations, using `std::expected` for error handling.

## Features

### Core Modules
- **path**: Extended path utilities, special directories, glob/regex matching
- **status**: File status, timestamps, symlink handling
- **find**: Powerful file search engine (like Linux `find`)
- **permissions**: POSIX permissions, ACL, Windows security descriptors, SID/Domain support
- **attributes**: Extended attributes (xattr), MIME type detection, file type identification
- **filesystem**: Filesystem type detection, volume info, network shares, cloud storage
- **disk**: Partition tables, disk enumeration, mount information, S.M.A.R.T. support
- **file_lock**: Cross-platform file locking (exclusive/shared)
- **watcher**: File system monitoring (like chokidar)
- **async**: Asynchronous operations based on stdexec (C++23 sender/receiver)

### Highlights
- **Modern C++23**: Uses `std::expected<T, fs_error>` for all error handling
- **Cross-platform**: Windows, Linux, macOS support
- **No exceptions**: All functions are `noexcept` with result types
- **Async support**: Built on NVIDIA stdexec for sender/receiver model
- **File Monitoring**: Real-time file change detection with callbacks
- **File Locking**: RAII-based exclusive and shared locks

## Requirements

- C++23 compatible compiler (Clang 16+, GCC 13+, MSVC 19.36+)
- CMake 3.20+

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build . -j8
ctest --output-on-failure
```

## Quick Start

### Path Operations
```cpp
#include <frappe/path.hpp>

auto home = frappe::home_path();
auto exe = frappe::executable_path();
auto docs = frappe::documents_path();

// Glob matching
auto cpp_files = frappe::glob("/src", "**/*.cpp");

// Regex matching
auto re = frappe::compile_regex(R"(.*\.hpp)");
```

### File Status
```cpp
#include <frappe/status.hpp>

auto status = frappe::get_status("/path/to/file");
if (status && status->is_regular_file()) {
    auto size = frappe::file_size("/path/to/file");
    auto mtime = frappe::last_write_time("/path/to/file");
}
```

### Permissions
```cpp
#include <frappe/permissions.hpp>

auto perms = frappe::get_permissions("/path/to/file");
auto owner = frappe::get_owner("/path/to/file");

// Windows SID support
auto sid = frappe::get_sid("username");
auto file_owner_sid = frappe::get_file_owner_sid("/path/to/file");

// ACL support
auto acl = frappe::get_acl("/path/to/file");
```

### Disk Information
```cpp
#include <frappe/disk.hpp>

auto disks = frappe::list_disks();
for (const auto& disk : *disks) {
    std::cout << disk.model << " - " << disk.size << " bytes\n";
}

auto partitions = frappe::list_partitions();
auto mounts = frappe::list_mounts();
auto boot_disk = frappe::get_boot_disk();
```

### Filesystem Features
```cpp
#include <frappe/filesystem.hpp>

auto fs_type = frappe::get_filesystem_type("/path");
auto features = frappe::get_filesystem_features("/path");
auto volumes = frappe::list_volumes();

// Network share detection
if (frappe::is_network_path("\\\\server\\share")) {
    auto share_info = frappe::get_network_share_info("\\\\server\\share");
}
```

### File Locking
```cpp
#include <frappe/file_lock.hpp>

// RAII-style locking (recommended)
{
    frappe::scoped_file_lock lock("/path/to/file.txt");
    if (lock) {
        // File is locked - safe to read/write
    }
} // Lock automatically released

// Manual locking with options
frappe::file_lock lock("/path/to/file.txt", {
    .type = frappe::lock_type::exclusive,
    .mode = frappe::lock_mode::timeout,
    .timeout = std::chrono::seconds(5)
});
if (auto result = lock.lock(); result) {
    // Do work
    lock.unlock();
}
```

### File Watching
```cpp
#include <frappe/watcher.hpp>

frappe::file_watcher watcher;
watcher.add("/path/to/watch");

watcher.on_add([](const frappe::watch_event& e) {
    std::cout << "Created: " << e.file_path << '\n';
});

watcher.on_change([](const frappe::watch_event& e) {
    std::cout << "Modified: " << e.file_path << '\n';
});

watcher.start();  // Runs in background thread
// ... do other work ...
watcher.stop();
```

### Async Operations
```cpp
#include <frappe/async.hpp>

// Async file search
auto sender = frappe::find_sender("/home/user", 
    frappe::find_options{}.is_file().name("*.cpp"));
auto result = frappe::sync_wait(std::move(sender));

// Execute on thread pool
auto pooled = frappe::on_thread_pool(frappe::enumerate_disks_sender());

// Chained operations
auto chain = frappe::exists_sender("/path")
    | stdexec::then([](auto result) {
        if (result && *result) {
            return frappe::file_size_sender("/path");
        }
        return stdexec::just(frappe::result<std::uintmax_t>{});
    });
```

## API Overview

| Module | Key Functions |
|--------|---------------|
| path | `home_directory`, `executable_path`, `glob`, `regex_match` |
| status | `status`, `file_size`, `get_file_times`, `exists` |
| find | `find`, `find_count`, `find_each`, `find_options` |
| permissions | `get_permissions`, `set_permissions`, `check_access` |
| attributes | `get_xattr`, `set_xattr`, `get_attributes`, `has_attribute` |
| filesystem | `space`, `get_filesystem_type`, `get_volume_info`, `is_network_path` |
| disk | `enumerate_disks`, `get_disk_info`, `get_partition_table` |
| file_lock | `file_lock`, `scoped_file_lock`, `lock_file`, `try_lock_file` |
| watcher | `file_watcher`, `wait_for_change`, `is_watch_supported` |
| async | `*_sender` variants, `sync_wait`, `on_thread_pool` |

## Documentation

- [API Reference](docs/API_REFERENCE.md)
- [Quick Start Guide](docs/QUICKSTART.md)
- [Platform Notes](docs/PLATFORM_NOTES.md)

## Test Coverage

```
100% tests passed, 0 tests failed out of 268
```

## License

MIT License
