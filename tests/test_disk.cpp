#include "frappe/disk.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

class DiskTest : public ::testing::Test {
  protected:
    void SetUp() override { _test_path = std::filesystem::temp_directory_path(); }

    std::filesystem::path _test_path;
};

// ============================================================================
// 7.1 Partition Table Type Tests
// ============================================================================

TEST_F(DiskTest, PartitionTableTypeName) {
    EXPECT_EQ(frappe::partition_table_type_name(frappe::partition_table_type::mbr), "mbr");
    EXPECT_EQ(frappe::partition_table_type_name(frappe::partition_table_type::gpt), "gpt");
    EXPECT_EQ(frappe::partition_table_type_name(frappe::partition_table_type::apm), "apm");
    EXPECT_EQ(frappe::partition_table_type_name(frappe::partition_table_type::unknown), "unknown");
}

// ============================================================================
// 7.2 Partition Type Tests
// ============================================================================

TEST_F(DiskTest, PartitionTypeName) {
    EXPECT_EQ(frappe::partition_type_name(frappe::partition_type::efi_system), "efi_system");
    EXPECT_EQ(frappe::partition_type_name(frappe::partition_type::microsoft_basic_data), "microsoft_basic_data");
    EXPECT_EQ(frappe::partition_type_name(frappe::partition_type::linux_filesystem), "linux_filesystem");
    EXPECT_EQ(frappe::partition_type_name(frappe::partition_type::apple_apfs), "apple_apfs");
    EXPECT_EQ(frappe::partition_type_name(frappe::partition_type::unknown), "unknown");
}

// ============================================================================
// 7.6 Disk Type Tests
// ============================================================================

TEST_F(DiskTest, DiskTypeName) {
    EXPECT_EQ(frappe::disk_type_name(frappe::disk_type::hdd), "hdd");
    EXPECT_EQ(frappe::disk_type_name(frappe::disk_type::ssd), "ssd");
    EXPECT_EQ(frappe::disk_type_name(frappe::disk_type::nvme), "nvme");
    EXPECT_EQ(frappe::disk_type_name(frappe::disk_type::usb), "usb");
    EXPECT_EQ(frappe::disk_type_name(frappe::disk_type::unknown), "unknown");
}

TEST_F(DiskTest, DiskBusTypeName) {
    EXPECT_EQ(frappe::disk_bus_type_name(frappe::disk_bus_type::sata), "sata");
    EXPECT_EQ(frappe::disk_bus_type_name(frappe::disk_bus_type::nvme), "nvme");
    EXPECT_EQ(frappe::disk_bus_type_name(frappe::disk_bus_type::usb), "usb");
    EXPECT_EQ(frappe::disk_bus_type_name(frappe::disk_bus_type::unknown), "unknown");
}

// ============================================================================
// 7.7 Disk Enumeration Tests
// ============================================================================

TEST_F(DiskTest, ListDisks) {
    auto disks = frappe::list_disks();
    EXPECT_TRUE(disks.has_value());
    EXPECT_GT(disks->size(), 0u);

    for (const auto &disk : *disks) {
        EXPECT_FALSE(disk.device_path.empty());
        EXPECT_GT(disk.size, 0u);
    }
}

TEST_F(DiskTest, GetDiskForPath) {
    auto disk = frappe::get_disk_for_path(_test_path);
    EXPECT_TRUE(disk.has_value());
    EXPECT_FALSE(disk->device_path.empty());
    EXPECT_GT(disk->size, 0u);
}

TEST_F(DiskTest, GetDiskInfo) {
    auto disks = frappe::list_disks();
    ASSERT_TRUE(disks.has_value());
    ASSERT_GT(disks->size(), 0u);

    auto info = frappe::get_disk_info(disks->front().device_path);
    EXPECT_TRUE(info.has_value());
    EXPECT_EQ(info->device_path, disks->front().device_path);
}

// ============================================================================
// Partition Table Tests
// ============================================================================

TEST_F(DiskTest, GetPartitionTable) {
    auto disks = frappe::list_disks();
    ASSERT_TRUE(disks.has_value());
    ASSERT_GT(disks->size(), 0u);

    auto table = frappe::get_partition_table(disks->front().device_path);
    EXPECT_TRUE(table.has_value());
    EXPECT_FALSE(table->device_path.empty());
    EXPECT_NE(table->type, frappe::partition_table_type::unknown);
}

// ============================================================================
// Partition Enumeration Tests
// ============================================================================

TEST_F(DiskTest, ListPartitions) {
    auto partitions = frappe::list_partitions();
    EXPECT_TRUE(partitions.has_value());
    // Should have at least one partition (system partition)
    EXPECT_GT(partitions->size(), 0u);
}

TEST_F(DiskTest, ListMountedPartitions) {
    auto partitions = frappe::list_mounted_partitions();
    EXPECT_TRUE(partitions.has_value());
    // Should have at least one mounted partition
    EXPECT_GT(partitions->size(), 0u);

    for (const auto &p : *partitions) {
        EXPECT_FALSE(p.mount_point.empty());
    }
}

TEST_F(DiskTest, GetPartitionInfo) {
    auto info = frappe::get_partition_info(_test_path);
    EXPECT_TRUE(info.has_value());
    EXPECT_GT(info->size, 0u);
}

// ============================================================================
// Containing Device Tests
// ============================================================================

TEST_F(DiskTest, GetContainingDevice) {
    auto device = frappe::get_containing_device(_test_path);
    EXPECT_TRUE(device.has_value());
    EXPECT_FALSE(device->empty());
}

// ============================================================================
// System/Boot Partition Tests
// ============================================================================

TEST_F(DiskTest, IsSystemPartition) {
    auto result = frappe::is_system_partition(_test_path);
    EXPECT_TRUE(result.has_value());
    // Temp directory is typically not on system partition
}

TEST_F(DiskTest, IsBootPartition) {
    auto result = frappe::is_boot_partition(_test_path);
    EXPECT_TRUE(result.has_value());
}

// ============================================================================
// Free Region Tests
// ============================================================================

TEST_F(DiskTest, GetFreeRegions) {
    auto disks = frappe::list_disks();
    ASSERT_TRUE(disks.has_value());
    ASSERT_GT(disks->size(), 0u);

    auto regions = frappe::get_free_regions(disks->front().device_path);
    EXPECT_TRUE(regions.has_value());
    // May or may not have free regions
}

TEST_F(DiskTest, GetTotalFreeSpace) {
    auto disks = frappe::list_disks();
    ASSERT_TRUE(disks.has_value());
    ASSERT_GT(disks->size(), 0u);

    auto free_space = frappe::get_total_free_space(disks->front().device_path);
    EXPECT_TRUE(free_space.has_value());
}

// ============================================================================
// Partition Search Tests
// ============================================================================

TEST_F(DiskTest, GetPartitionByLabel) {
    auto result = frappe::get_partition_by_label("NonExistentLabel12345");
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result->has_value()); // Should be nullopt
}

TEST_F(DiskTest, GetPartitionByUuid) {
    auto result = frappe::get_partition_by_uuid("00000000-0000-0000-0000-000000000000");
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result->has_value()); // Should be nullopt
}

// ============================================================================
// 7.12 Mount Information Tests
// ============================================================================

TEST_F(DiskTest, ListMounts) {
    auto mounts = frappe::list_mounts();
    EXPECT_TRUE(mounts.has_value());
    EXPECT_GT(mounts->size(), 0u);

    for (const auto &m : *mounts) {
        EXPECT_FALSE(m.mount_point.empty());
    }
}

TEST_F(DiskTest, GetMountInfo) {
    auto info = frappe::get_mount_info(_test_path);
    EXPECT_TRUE(info.has_value());
    EXPECT_FALSE(info->mount_point.empty());
}

TEST_F(DiskTest, IsMountPoint) {
#ifdef _WIN32
    auto result = frappe::is_mount_point("C:\\");
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(*result);
#else
    auto result = frappe::is_mount_point("/");
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(*result);
#endif
}

TEST_F(DiskTest, IsMounted) {
    auto mounts = frappe::list_mounts();
    ASSERT_TRUE(mounts.has_value());
    ASSERT_GT(mounts->size(), 0u);

    // First mount should be mounted
    auto result = frappe::is_mounted(mounts->front().device);
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(*result);
}

TEST_F(DiskTest, GetMountPoint) {
    auto mounts = frappe::list_mounts();
    ASSERT_TRUE(mounts.has_value());
    ASSERT_GT(mounts->size(), 0u);

    auto mp = frappe::get_mount_point(mounts->front().device);
    EXPECT_TRUE(mp.has_value());
    EXPECT_EQ(*mp, mounts->front().mount_point);
}

TEST_F(DiskTest, GetDeviceByMountPoint) {
    auto mounts = frappe::list_mounts();
    ASSERT_TRUE(mounts.has_value());
    ASSERT_GT(mounts->size(), 0u);

    auto device = frappe::get_device_by_mount_point(mounts->front().mount_point);
    EXPECT_TRUE(device.has_value());
    EXPECT_EQ(*device, mounts->front().device);
}

TEST_F(DiskTest, ListUnmountedPartitions) {
    auto partitions = frappe::list_unmounted_partitions();
    EXPECT_TRUE(partitions.has_value());
    // May or may not have unmounted partitions

    for (const auto &p : *partitions) {
        EXPECT_TRUE(p.mount_point.empty());
    }
}

TEST_F(DiskTest, GetBootDisk) {
    auto disk = frappe::get_boot_disk();
    EXPECT_TRUE(disk.has_value());
    EXPECT_FALSE(disk->device_path.empty());
}

TEST_F(DiskTest, GetSystemDisk) {
    auto disk = frappe::get_system_disk();
    EXPECT_TRUE(disk.has_value());
    EXPECT_FALSE(disk->device_path.empty());
}

// ============================================================================
// 7.8 Extended Disk Query Functions
// ============================================================================

TEST_F(DiskTest, ListPhysicalDisks) {
    auto disks = frappe::list_physical_disks();
    EXPECT_TRUE(disks.has_value());

    for (const auto &d : *disks) {
        EXPECT_NE(d.type, frappe::disk_type::virtual_disk);
        EXPECT_NE(d.type, frappe::disk_type::ram_disk);
    }
}

TEST_F(DiskTest, ListRemovableDisks) {
    auto disks = frappe::list_removable_disks();
    EXPECT_TRUE(disks.has_value());

    for (const auto &d : *disks) {
        EXPECT_TRUE(d.is_removable);
    }
}

TEST_F(DiskTest, GetDiskBySerial) {
    auto disks = frappe::list_disks();
    ASSERT_TRUE(disks.has_value());

    if (!disks->empty() && !disks->front().serial.empty()) {
        auto disk = frappe::get_disk_by_serial(disks->front().serial);
        EXPECT_TRUE(disk.has_value());
        EXPECT_TRUE(disk->has_value());
        EXPECT_EQ((*disk)->serial, disks->front().serial);
    }
}

// ============================================================================
// 7.9 S.M.A.R.T. Information
// ============================================================================

TEST_F(DiskTest, SmartStatusName) {
    EXPECT_EQ(frappe::smart_status_name(frappe::smart_status::good), "good");
    EXPECT_EQ(frappe::smart_status_name(frappe::smart_status::warning), "warning");
    EXPECT_EQ(frappe::smart_status_name(frappe::smart_status::critical), "critical");
    EXPECT_EQ(frappe::smart_status_name(frappe::smart_status::failing), "failing");
    EXPECT_EQ(frappe::smart_status_name(frappe::smart_status::unknown), "unknown");
}

TEST_F(DiskTest, GetSmartInfo) {
    auto disks = frappe::list_disks();
    ASSERT_TRUE(disks.has_value());

    if (!disks->empty()) {
        auto smart = frappe::get_smart_info(disks->front().device_path);
        // May or may not succeed depending on permissions
        if (smart.has_value()) {
            // If supported, health should be set
            if (smart->is_supported) {
                EXPECT_NE(smart->health_status, frappe::smart_status::unknown);
            }
        }
    }
}

TEST_F(DiskTest, GetDiskHealth) {
    auto disks = frappe::list_disks();
    ASSERT_TRUE(disks.has_value());

    if (!disks->empty()) {
        auto health = frappe::get_disk_health(disks->front().device_path);
        // May fail due to permissions, that's OK
    }
}

// ============================================================================
// 7.10 NVMe Information
// ============================================================================

TEST_F(DiskTest, IsNvmeDevice) {
    auto disks = frappe::list_disks();
    ASSERT_TRUE(disks.has_value());

    for (const auto &d : *disks) {
        auto is_nvme = frappe::is_nvme_device(d.device_path);
        if (is_nvme.has_value()) {
            if (*is_nvme) {
                EXPECT_TRUE(d.type == frappe::disk_type::nvme || d.bus_type == frappe::disk_bus_type::nvme);
            }
        }
    }
}

// ============================================================================
// 7.11 Storage Pool / RAID Information
// ============================================================================

TEST_F(DiskTest, StoragePoolTypeName) {
    EXPECT_EQ(frappe::storage_pool_type_name(frappe::storage_pool_type::lvm), "lvm");
    EXPECT_EQ(frappe::storage_pool_type_name(frappe::storage_pool_type::zfs), "zfs");
    EXPECT_EQ(frappe::storage_pool_type_name(frappe::storage_pool_type::storage_spaces), "storage_spaces");
    EXPECT_EQ(frappe::storage_pool_type_name(frappe::storage_pool_type::unknown), "unknown");
}

TEST_F(DiskTest, RaidLevelName) {
    EXPECT_EQ(frappe::raid_level_name(frappe::raid_level::raid0), "raid0");
    EXPECT_EQ(frappe::raid_level_name(frappe::raid_level::raid1), "raid1");
    EXPECT_EQ(frappe::raid_level_name(frappe::raid_level::raid5), "raid5");
    EXPECT_EQ(frappe::raid_level_name(frappe::raid_level::raid10), "raid10");
    EXPECT_EQ(frappe::raid_level_name(frappe::raid_level::unknown), "unknown");
}

TEST_F(DiskTest, RaidStateName) {
    EXPECT_EQ(frappe::raid_state_name(frappe::raid_state::active), "active");
    EXPECT_EQ(frappe::raid_state_name(frappe::raid_state::degraded), "degraded");
    EXPECT_EQ(frappe::raid_state_name(frappe::raid_state::rebuilding), "rebuilding");
    EXPECT_EQ(frappe::raid_state_name(frappe::raid_state::unknown), "unknown");
}

TEST_F(DiskTest, ListStoragePools) {
    auto pools = frappe::list_storage_pools();
    EXPECT_TRUE(pools.has_value());
    // May be empty if no storage pools configured
}

TEST_F(DiskTest, ListRaidArrays) {
    auto arrays = frappe::list_raid_arrays();
    EXPECT_TRUE(arrays.has_value());
    // May be empty if no RAID configured
}

// ============================================================================
// 7.13 Virtual Disk Support
// ============================================================================

TEST_F(DiskTest, VirtualDiskTypeName) {
    EXPECT_EQ(frappe::virtual_disk_type_name(frappe::virtual_disk_type::vhd), "vhd");
    EXPECT_EQ(frappe::virtual_disk_type_name(frappe::virtual_disk_type::vhdx), "vhdx");
    EXPECT_EQ(frappe::virtual_disk_type_name(frappe::virtual_disk_type::vmdk), "vmdk");
    EXPECT_EQ(frappe::virtual_disk_type_name(frappe::virtual_disk_type::vdi), "vdi");
    EXPECT_EQ(frappe::virtual_disk_type_name(frappe::virtual_disk_type::qcow2), "qcow2");
    EXPECT_EQ(frappe::virtual_disk_type_name(frappe::virtual_disk_type::iso), "iso");
    EXPECT_EQ(frappe::virtual_disk_type_name(frappe::virtual_disk_type::dmg), "dmg");
    EXPECT_EQ(frappe::virtual_disk_type_name(frappe::virtual_disk_type::unknown), "unknown");
}

TEST_F(DiskTest, IsVirtualDisk) {
    EXPECT_TRUE(*frappe::is_virtual_disk("test.vhd"));
    EXPECT_TRUE(*frappe::is_virtual_disk("test.vhdx"));
    EXPECT_TRUE(*frappe::is_virtual_disk("test.vmdk"));
    EXPECT_TRUE(*frappe::is_virtual_disk("test.vdi"));
    EXPECT_TRUE(*frappe::is_virtual_disk("test.qcow2"));
    EXPECT_TRUE(*frappe::is_virtual_disk("test.iso"));
    EXPECT_TRUE(*frappe::is_virtual_disk("test.dmg"));
    EXPECT_TRUE(*frappe::is_virtual_disk("test.img"));
    EXPECT_FALSE(*frappe::is_virtual_disk("test.txt"));
    EXPECT_FALSE(*frappe::is_virtual_disk("test.exe"));
}

TEST_F(DiskTest, GetVirtualDiskType) {
    EXPECT_EQ(*frappe::get_virtual_disk_type("test.vhd"), frappe::virtual_disk_type::vhd);
    EXPECT_EQ(*frappe::get_virtual_disk_type("test.vhdx"), frappe::virtual_disk_type::vhdx);
    EXPECT_EQ(*frappe::get_virtual_disk_type("test.vmdk"), frappe::virtual_disk_type::vmdk);
    EXPECT_EQ(*frappe::get_virtual_disk_type("test.vdi"), frappe::virtual_disk_type::vdi);
    EXPECT_EQ(*frappe::get_virtual_disk_type("test.qcow2"), frappe::virtual_disk_type::qcow2);
    EXPECT_EQ(*frappe::get_virtual_disk_type("test.iso"), frappe::virtual_disk_type::iso);
    EXPECT_EQ(*frappe::get_virtual_disk_type("test.dmg"), frappe::virtual_disk_type::dmg);
    EXPECT_EQ(*frappe::get_virtual_disk_type("test.txt"), frappe::virtual_disk_type::unknown);
}

TEST_F(DiskTest, ListMountedVirtualDisks) {
    auto disks = frappe::list_mounted_virtual_disks();
    EXPECT_TRUE(disks.has_value());
    // May be empty if no virtual disks mounted
}

TEST_F(DiskTest, ListVirtualDiskFiles) {
    // Create a temp directory with a fake virtual disk file
    auto temp_dir = std::filesystem::temp_directory_path() / "frappe_vdisk_test";
    std::filesystem::create_directories(temp_dir);

    auto fake_vhd = temp_dir / "test.vhd";
    std::ofstream(fake_vhd) << "fake vhd content";

    auto files = frappe::list_virtual_disk_files(temp_dir);
    EXPECT_TRUE(files.has_value());
    EXPECT_EQ(files->size(), 1u);

    std::filesystem::remove_all(temp_dir);
}
