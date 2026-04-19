#ifndef FRAPPE_ASYNC_HPP
#define FRAPPE_ASYNC_HPP

#include "frappe/error.hpp"
#include "frappe/find.hpp"
#include "frappe/disk.hpp"
#include "frappe/permissions.hpp"
#include "frappe/filesystem.hpp"
#include "frappe/file_lock.hpp"
#include "frappe/watcher.hpp"
#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>
#include <filesystem>
#include <vector>
#include <chrono>
#include <functional>
#include <atomic>

namespace frappe {

using path = std::filesystem::path;

// ============================================================================
// 7.5.1 Progress Information
// ============================================================================

struct progress_info {
    std::uint64_t current = 0;
    std::uint64_t total = 0;
    double percentage = 0.0;
    std::string current_item;
    std::chrono::milliseconds elapsed_time{0};
    std::chrono::milliseconds estimated_remaining{0};
};

using progress_callback = std::function<void(const progress_info&)>;

// ============================================================================
// 7.5.2 Thread Pool Scheduler
// ============================================================================

inline exec::static_thread_pool& get_default_thread_pool() {
    static exec::static_thread_pool pool{std::thread::hardware_concurrency()};
    return pool;
}

inline auto get_default_scheduler() {
    return get_default_thread_pool().get_scheduler();
}

// ============================================================================
// 7.5.3 Async Sender Factories
// ============================================================================

// List disks asynchronously
inline auto list_disks_sender() {
    return stdexec::just() 
        | stdexec::then([]() {
            return list_disks();
        });
}

// Get disk info asynchronously
inline auto get_disk_info_sender(std::string device_path) {
    return stdexec::just(std::move(device_path))
        | stdexec::then([](std::string path) {
            return get_disk_info(path);
        });
}

// List partitions asynchronously
inline auto list_partitions_sender() {
    return stdexec::just()
        | stdexec::then([]() {
            return list_partitions();
        });
}

// Get partition table asynchronously
inline auto get_partition_table_sender(std::string device_path) {
    return stdexec::just(std::move(device_path))
        | stdexec::then([](std::string path) {
            return get_partition_table(path);
        });
}

// List mounts asynchronously
inline auto list_mounts_sender() {
    return stdexec::just()
        | stdexec::then([]() {
            return list_mounts();
        });
}

// Get file owner SID asynchronously (Windows)
inline auto get_file_owner_sid_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path file_path) {
            return get_file_owner_sid(file_path);
        });
}

// Get permissions asynchronously
inline auto get_permissions_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path file_path) {
            return get_permissions(file_path);
        });
}

// Check access asynchronously
inline auto check_access_sender(path p, access_rights rights) {
    return stdexec::just(std::move(p), rights)
        | stdexec::then([](path file_path, access_rights r) {
            return check_access(file_path, r);
        });
}

// ============================================================================
// 7.5.4 Batch Operations
// ============================================================================

// Get permissions for multiple paths
inline auto get_permissions_bulk_sender(std::vector<path> paths) {
    return stdexec::just(std::move(paths))
        | stdexec::then([](std::vector<path> file_paths) {
            std::vector<result<perms>> results;
            results.reserve(file_paths.size());
            for (const auto& p : file_paths) {
                results.push_back(get_permissions(p));
            }
            return results;
        });
}

// Check access for multiple paths
inline auto check_access_bulk_sender(std::vector<path> paths, access_rights rights) {
    return stdexec::just(std::move(paths), rights)
        | stdexec::then([](std::vector<path> file_paths, access_rights r) {
            std::vector<result<bool>> results;
            results.reserve(file_paths.size());
            for (const auto& p : file_paths) {
                results.push_back(check_access(p, r));
            }
            return results;
        });
}

// ============================================================================
// 7.5.5 Scheduler-based execution
// ============================================================================

// Execute sender on thread pool
template<typename Sender>
auto on_thread_pool(Sender&& sender) {
    return stdexec::on(get_default_scheduler(), std::forward<Sender>(sender));
}

// Execute sender and wait for result
template<typename Sender>
auto sync_wait(Sender&& sender) {
    return stdexec::sync_wait(std::forward<Sender>(sender));
}

// ============================================================================
// 7.5.2 搜索操作Sender
// ============================================================================

// Find files asynchronously
inline auto find_sender(path search_path, find_options options = {}) {
    return stdexec::just(std::move(search_path), std::move(options))
        | stdexec::then([](path p, find_options opts) {
            return find(p, opts);
        });
}

// Find first match asynchronously
inline auto find_first_sender(path search_path, find_options options) {
    return stdexec::just(std::move(search_path), std::move(options))
        | stdexec::then([](path p, find_options opts) {
            return find_first(p, opts);
        });
}

// Count matches asynchronously
inline auto find_count_sender(path search_path, find_options options) {
    return stdexec::just(std::move(search_path), std::move(options))
        | stdexec::then([](path p, find_options opts) {
            return find_count(p, opts);
        });
}

// ============================================================================
// 7.5.4 网络操作Sender
// ============================================================================

// List network shares asynchronously
inline auto list_network_shares_sender() {
    return stdexec::just()
        | stdexec::then([]() {
            return list_network_shares();
        });
}

// Get network share info asynchronously
inline auto get_network_share_info_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path file_path) {
            return get_network_share_info(file_path);
        });
}

// Check if network path asynchronously
inline auto is_network_path_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path file_path) {
            return is_network_path(file_path);
        });
}

#ifdef _WIN32
// List mapped drives asynchronously (Windows only)
inline auto list_mapped_drives_sender() {
    return stdexec::just()
        | stdexec::then([]() {
            return list_mapped_drives();
        });
}
#endif

// ============================================================================
// 7.5.5 可移动设备Sender
// ============================================================================

// List removable devices asynchronously
inline auto list_removable_devices_sender() {
    return stdexec::just()
        | stdexec::then([]() {
            return list_removable_devices();
        });
}

// Get removable device info asynchronously
inline auto get_removable_device_info_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path device_path) {
            return get_removable_device_info(device_path);
        });
}

// Check if device is ready asynchronously
inline auto is_device_ready_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path device_path) {
            return is_device_ready(device_path);
        });
}

// Check if media is present asynchronously
inline auto is_media_present_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path device_path) {
            return is_media_present(device_path);
        });
}

// ============================================================================
// 7.5.6 云存储Sender
// ============================================================================

// Check if cloud path asynchronously
inline auto is_cloud_path_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path file_path) {
            return is_cloud_path(file_path);
        });
}

// Get cloud provider asynchronously
inline auto get_cloud_provider_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path file_path) {
            return get_cloud_provider(file_path);
        });
}

// Get cloud storage info asynchronously
inline auto get_cloud_storage_info_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path file_path) {
            return get_cloud_storage_info(file_path);
        });
}

// List cloud mounts asynchronously
inline auto list_cloud_mounts_sender() {
    return stdexec::just()
        | stdexec::then([]() {
            return list_cloud_mounts();
        });
}

// ============================================================================
// 7.5.7 虚拟文件系统Sender
// ============================================================================

// Check if virtual filesystem asynchronously
inline auto is_virtual_filesystem_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path file_path) {
            return is_virtual_filesystem(file_path);
        });
}

// Check if FUSE mount asynchronously
inline auto is_fuse_mount_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path file_path) {
            return is_fuse_mount(file_path);
        });
}

// Check if encrypted mount asynchronously
inline auto is_encrypted_mount_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path file_path) {
            return is_encrypted_mount(file_path);
        });
}

// Get virtual filesystem info asynchronously
inline auto get_virtual_fs_info_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path file_path) {
            return get_virtual_fs_info(file_path);
        });
}

// ============================================================================
// 7.5.8 文件系统操作Sender
// ============================================================================

// Get filesystem type asynchronously
inline auto get_filesystem_type_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path file_path) {
            return get_filesystem_type(file_path);
        });
}

// Get filesystem features asynchronously
inline auto get_filesystem_features_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path file_path) {
            return get_filesystem_features(file_path);
        });
}

// Get volume info asynchronously
inline auto get_volume_info_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path file_path) {
            return get_volume_info(file_path);
        });
}

// List volumes asynchronously
inline auto list_volumes_sender() {
    return stdexec::just()
        | stdexec::then([]() {
            return list_volumes();
        });
}

// Get space info asynchronously
inline auto space_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path file_path) {
            return frappe::space(file_path);
        });
}

// ============================================================================
// 7.5.9 批量操作Sender（扩展）
// ============================================================================

// Get filesystem type for multiple paths
inline auto get_filesystem_type_bulk_sender(std::vector<path> paths) {
    return stdexec::just(std::move(paths))
        | stdexec::then([](std::vector<path> file_paths) {
            std::vector<result<filesystem_type>> results;
            results.reserve(file_paths.size());
            for (const auto& p : file_paths) {
                results.push_back(get_filesystem_type(p));
            }
            return results;
        });
}

// Check if cloud path for multiple paths
inline auto is_cloud_path_bulk_sender(std::vector<path> paths) {
    return stdexec::just(std::move(paths))
        | stdexec::then([](std::vector<path> file_paths) {
            std::vector<result<bool>> results;
            results.reserve(file_paths.size());
            for (const auto& p : file_paths) {
                results.push_back(is_cloud_path(p));
            }
            return results;
        });
}

// ============================================================================
// 9.1 文件锁定Sender
// ============================================================================

// Lock file asynchronously
inline auto lock_file_sender(path p, lock_options options = {}) {
    return stdexec::just(std::move(p), std::move(options))
        | stdexec::then([](path file_path, lock_options opts) {
            return lock_file(file_path, opts);
        });
}

// Try lock file asynchronously
inline auto try_lock_file_sender(path p, lock_type type = lock_type::exclusive) {
    return stdexec::just(std::move(p), type)
        | stdexec::then([](path file_path, lock_type t) {
            return try_lock_file(file_path, t);
        });
}

// Check if file is locked asynchronously
inline auto is_file_locked_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path file_path) {
            return is_file_locked(file_path);
        });
}

// ============================================================================
// 9.2 文件监控Sender
// ============================================================================

// Wait for file change asynchronously
inline auto wait_for_change_sender(path p, std::chrono::milliseconds timeout = std::chrono::milliseconds{0}) {
    return stdexec::just(std::move(p), timeout)
        | stdexec::then([](path file_path, std::chrono::milliseconds t) {
            return wait_for_change(file_path, t);
        });
}

// Check if watch is supported asynchronously
inline auto is_watch_supported_sender(path p) {
    return stdexec::just(std::move(p))
        | stdexec::then([](path file_path) {
            return is_watch_supported(file_path);
        });
}

} // namespace frappe

#endif // FRAPPE_ASYNC_HPP
