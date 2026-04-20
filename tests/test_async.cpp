#include "frappe/async.hpp"

#include <filesystem>
#include <gtest/gtest.h>

class AsyncTest : public ::testing::Test {
  protected:
    void SetUp() override { _test_path = std::filesystem::temp_directory_path(); }

    std::filesystem::path _test_path;
};

// ============================================================================
// Progress Info Tests
// ============================================================================

TEST_F(AsyncTest, ProgressInfoDefault) {
    frappe::progress_info info;
    EXPECT_EQ(info.current, 0u);
    EXPECT_EQ(info.total, 0u);
    EXPECT_EQ(info.percentage, 0.0);
    EXPECT_TRUE(info.current_item.empty());
}

// ============================================================================
// Thread Pool Tests
// ============================================================================

TEST_F(AsyncTest, GetDefaultThreadPool) {
    auto &pool = frappe::get_default_thread_pool();
    (void)pool; // Just verify it doesn't crash
}

TEST_F(AsyncTest, GetDefaultScheduler) {
    auto scheduler = frappe::get_default_scheduler();
    (void)scheduler; // Just verify it doesn't crash
}

// ============================================================================
// Disk Sender Tests
// ============================================================================

TEST_F(AsyncTest, ListDisksSender) {
    auto sender = frappe::list_disks_sender();
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[disks] = *result;
    EXPECT_TRUE(disks.has_value());
    EXPECT_GT(disks->size(), 0u);
}

TEST_F(AsyncTest, ListPartitionsSender) {
    auto sender = frappe::list_partitions_sender();
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[partitions] = *result;
    EXPECT_TRUE(partitions.has_value());
}

TEST_F(AsyncTest, ListMountsSender) {
    auto sender = frappe::list_mounts_sender();
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[mounts] = *result;
    EXPECT_TRUE(mounts.has_value());
    EXPECT_GT(mounts->size(), 0u);
}

TEST_F(AsyncTest, GetDiskInfoSender) {
    // First get a disk
    auto disks = frappe::list_disks();
    ASSERT_TRUE(disks.has_value());
    ASSERT_GT(disks->size(), 0u);

    auto sender = frappe::get_disk_info_sender(disks->front().device_path);
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[info] = *result;
    EXPECT_TRUE(info.has_value());
    EXPECT_EQ(info->device_path, disks->front().device_path);
}

TEST_F(AsyncTest, GetPartitionTableSender) {
    auto disks = frappe::list_disks();
    ASSERT_TRUE(disks.has_value());
    ASSERT_GT(disks->size(), 0u);

    auto sender = frappe::get_partition_table_sender(disks->front().device_path);
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[table] = *result;
    EXPECT_TRUE(table.has_value());
}

// ============================================================================
// Permission Sender Tests
// ============================================================================

TEST_F(AsyncTest, GetPermissionsSender) {
    auto sender = frappe::get_permissions_sender(_test_path);
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[perms] = *result;
    EXPECT_TRUE(perms.has_value());
}

TEST_F(AsyncTest, CheckAccessSender) {
    auto sender = frappe::check_access_sender(_test_path, frappe::access_rights::read);
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[can_access] = *result;
    EXPECT_TRUE(can_access.has_value());
    EXPECT_TRUE(*can_access);
}

// ============================================================================
// Bulk Operation Tests
// ============================================================================

TEST_F(AsyncTest, GetPermissionsBulkSender) {
    std::vector<frappe::path> paths = {_test_path, _test_path};

    auto sender = frappe::get_permissions_bulk_sender(std::move(paths));
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[perms_list] = *result;
    EXPECT_EQ(perms_list.size(), 2u);
    for (const auto &p : perms_list) {
        EXPECT_TRUE(p.has_value());
    }
}

TEST_F(AsyncTest, CheckAccessBulkSender) {
    std::vector<frappe::path> paths = {_test_path, _test_path};

    auto sender = frappe::check_access_bulk_sender(std::move(paths), frappe::access_rights::read);
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[access_list] = *result;
    EXPECT_EQ(access_list.size(), 2u);
    for (const auto &a : access_list) {
        EXPECT_TRUE(a.has_value());
        EXPECT_TRUE(*a);
    }
}

// ============================================================================
// Thread Pool Execution Tests
// ============================================================================

TEST_F(AsyncTest, OnThreadPool) {
    auto sender = frappe::on_thread_pool(frappe::list_mounts_sender());
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[mounts] = *result;
    EXPECT_TRUE(mounts.has_value());
}

// ============================================================================
// Chained Operations Tests
// ============================================================================

TEST_F(AsyncTest, ChainedSenders) {
    auto sender = frappe::list_disks_sender() | stdexec::then([](frappe::result<std::vector<frappe::disk_info>> disks) {
                      if (disks.has_value() && !disks->empty()) {
                          return disks->front().device_path;
                      }
                      return std::string{};
                  });

    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[device_path] = *result;
    EXPECT_FALSE(device_path.empty());
}

// ============================================================================
// 7.5.2 搜索操作Sender Tests
// ============================================================================

TEST_F(AsyncTest, FindSender) {
    auto sender = frappe::find_sender(_test_path, frappe::find_options{}.is_file());
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[paths] = *result;
    EXPECT_GE(paths.size(), 1u);
}

TEST_F(AsyncTest, FindCountSender) {
    auto sender = frappe::find_count_sender(_test_path, frappe::find_options{}.is_file());
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[count] = *result;
    EXPECT_GE(count, 1u);
}

// ============================================================================
// 7.5.4 网络操作Sender Tests
// ============================================================================

TEST_F(AsyncTest, ListNetworkSharesSender) {
    auto sender = frappe::list_network_shares_sender();
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[shares] = *result;
    EXPECT_TRUE(shares.has_value());
}

TEST_F(AsyncTest, IsNetworkPathSender) {
    auto sender = frappe::is_network_path_sender(_test_path);
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[is_network] = *result;
    EXPECT_TRUE(is_network.has_value());
    EXPECT_FALSE(*is_network);
}

// ============================================================================
// 7.5.5 可移动设备Sender Tests
// ============================================================================

TEST_F(AsyncTest, ListRemovableDevicesSender) {
    auto sender = frappe::list_removable_devices_sender();
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[devices] = *result;
    EXPECT_TRUE(devices.has_value());
}

TEST_F(AsyncTest, IsDeviceReadySender) {
    auto sender = frappe::is_device_ready_sender(_test_path);
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[ready] = *result;
    EXPECT_TRUE(ready.has_value());
    EXPECT_TRUE(*ready);
}

// ============================================================================
// 7.5.6 云存储Sender Tests
// ============================================================================

TEST_F(AsyncTest, IsCloudPathSender) {
    auto sender = frappe::is_cloud_path_sender(_test_path);
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[is_cloud] = *result;
    EXPECT_TRUE(is_cloud.has_value());
}

TEST_F(AsyncTest, ListCloudMountsSender) {
    auto sender = frappe::list_cloud_mounts_sender();
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[mounts] = *result;
    EXPECT_TRUE(mounts.has_value());
}

// ============================================================================
// 7.5.7 虚拟文件系统Sender Tests
// ============================================================================

TEST_F(AsyncTest, IsVirtualFilesystemSender) {
    auto sender = frappe::is_virtual_filesystem_sender(_test_path);
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[is_virtual] = *result;
    EXPECT_TRUE(is_virtual.has_value());
}

TEST_F(AsyncTest, IsFuseMountSender) {
    auto sender = frappe::is_fuse_mount_sender(_test_path);
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[is_fuse] = *result;
    EXPECT_TRUE(is_fuse.has_value());
    EXPECT_FALSE(*is_fuse);
}

// ============================================================================
// 7.5.8 文件系统操作Sender Tests
// ============================================================================

TEST_F(AsyncTest, GetFilesystemTypeSender) {
    auto sender = frappe::get_filesystem_type_sender(_test_path);
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[fs_type] = *result;
    EXPECT_TRUE(fs_type.has_value());
}

TEST_F(AsyncTest, ListVolumesSender) {
    auto sender = frappe::list_volumes_sender();
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[volumes] = *result;
    EXPECT_TRUE(volumes.has_value());
    EXPECT_FALSE(volumes->empty());
}

TEST_F(AsyncTest, SpaceSender) {
    auto sender = frappe::space_sender(_test_path);
    auto result = frappe::sync_wait(std::move(sender));
    ASSERT_TRUE(result.has_value());

    auto &[space_info] = *result;
    EXPECT_TRUE(space_info.has_value());
    EXPECT_GT(space_info->capacity, 0u);
}
