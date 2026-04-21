#ifndef FRAPPE_DISK_HPP
#define FRAPPE_DISK_HPP

#include "frappe/error.hpp"
#include "frappe/exports.hpp"
#include "frappe/filesystem.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace frappe {

    using path = std::filesystem::path;

    // ============================================================================
    // 7.1 Partition Table Types
    // ============================================================================

    enum class partition_table_type {
        unknown,
        mbr, // Master Boot Record
        gpt, // GUID Partition Table
        apm, // Apple Partition Map
        bsd, // BSD disklabel
        sun  // Sun VTOC
    };

    [[nodiscard]] FRAPPE_API std::string_view partition_table_type_name(partition_table_type type) noexcept;

    // ============================================================================
    // 7.2 Partition Types (GPT Type GUIDs / MBR Type IDs)
    // ============================================================================

    enum class partition_type {
        unknown,
        // EFI/System
        efi_system,
        bios_boot,
        // Microsoft
        microsoft_reserved,
        microsoft_basic_data,
        microsoft_ldm_metadata,
        microsoft_ldm_data,
        microsoft_recovery,
        microsoft_storage_spaces,
        // Linux
        linux_filesystem,
        linux_swap,
        linux_lvm,
        linux_raid,
        linux_home,
        linux_root_x86,
        linux_root_x86_64,
        linux_root_arm,
        linux_root_arm64,
        // Apple
        apple_hfs,
        apple_apfs,
        apple_boot,
        apple_raid,
        apple_label,
        // Other
        freebsd_boot,
        freebsd_data,
        freebsd_swap,
        solaris_root,
        solaris_swap
    };

    [[nodiscard]] FRAPPE_API std::string_view partition_type_name(partition_type type) noexcept;

    // ============================================================================
    // 7.3 Partition Information
    // ============================================================================

    struct partition_info {
        std::string device_path; // e.g., /dev/sda1, \\.\PhysicalDrive0
        std::uint32_t partition_number = 0;
        std::string label;        // Volume label
        std::string uuid;         // Partition UUID
        std::string type_uuid;    // Partition type UUID (GPT)
        std::uint8_t type_id = 0; // Partition type ID (MBR)
        partition_type type = partition_type::unknown;

        std::uint64_t start_sector = 0;
        std::uint64_t end_sector = 0;
        std::uint64_t sector_count = 0;
        std::uint64_t offset = 0; // Start offset in bytes
        std::uint64_t size = 0;   // Size in bytes

        filesystem_type fs_type = filesystem_type::unknown;

        bool is_bootable = false;
        bool is_active = false;
        bool is_primary = false;
        bool is_extended = false;
        bool is_logical = false;
        bool is_hidden = false;

        std::string mount_point; // If mounted
    };

    // ============================================================================
    // 7.4 Partition Table Information
    // ============================================================================

    struct partition_table_info {
        std::string device_path;
        partition_table_type type = partition_table_type::unknown;
        std::string disk_uuid;            // GPT disk GUID
        std::uint32_t disk_signature = 0; // MBR signature

        std::uint64_t disk_size = 0;
        std::uint32_t sector_size = 512;
        std::uint32_t physical_sector_size = 512;
        std::uint64_t total_sectors = 0;
        std::uint64_t first_usable_sector = 0;
        std::uint64_t last_usable_sector = 0;

        std::vector<partition_info> partitions;
    };

    // ============================================================================
    // 7.5 Free Region Information
    // ============================================================================

    struct free_region {
        std::uint64_t start_sector = 0;
        std::uint64_t end_sector = 0;
        std::uint64_t size = 0;       // Size in bytes
        bool is_primary_slot = false; // Can create primary partition
    };

    // ============================================================================
    // 7.6 Disk Information
    // ============================================================================

    enum class disk_type {
        unknown,
        hdd,          // Hard Disk Drive
        ssd,          // Solid State Drive
        nvme,         // NVMe SSD
        usb,          // USB Drive
        optical,      // CD/DVD/Blu-ray
        floppy,       // Floppy disk
        virtual_disk, // Virtual disk
        ram_disk,     // RAM disk
        network       // iSCSI, etc.
    };

    enum class disk_bus_type {
        unknown,
        ata,
        sata,
        scsi,
        sas,
        usb,
        nvme,
        pcie,
        thunderbolt,
        firewire,
        virtual_bus,
        mmc, // SD/MMC
        raid
    };

    struct disk_info {
        std::string device_path; // e.g., /dev/sda, \\.\PhysicalDrive0
        std::string model;
        std::string vendor;
        std::string serial;
        std::string firmware_version;

        disk_type type = disk_type::unknown;
        disk_bus_type bus_type = disk_bus_type::unknown;

        std::uint64_t size = 0; // Total size in bytes
        std::uint32_t sector_size = 512;
        std::uint32_t physical_sector_size = 512;
        std::uint64_t total_sectors = 0;

        bool is_removable = false;
        bool is_readonly = false;
        bool is_system_disk = false;
        bool is_boot_disk = false;
        bool has_media = true; // For optical/removable

        partition_table_type partition_table = partition_table_type::unknown;
        std::uint32_t partition_count = 0;
    };

    [[nodiscard]] FRAPPE_API std::string_view disk_type_name(disk_type type) noexcept;
    [[nodiscard]] FRAPPE_API std::string_view disk_bus_type_name(disk_bus_type type) noexcept;

    // ============================================================================
    // 7.7 Disk and Partition Query Functions
    // ============================================================================

    // Disk enumeration
    [[nodiscard]] FRAPPE_API result<std::vector<disk_info>> list_disks() noexcept;
    [[nodiscard]] FRAPPE_API result<disk_info> get_disk_info(std::string_view device_path) noexcept;
    [[nodiscard]] FRAPPE_API result<disk_info> get_disk_for_path(const path &p) noexcept;

    // Partition table
    [[nodiscard]] FRAPPE_API result<partition_table_info> get_partition_table(std::string_view device_path) noexcept;
    [[nodiscard]] FRAPPE_API result<partition_info> get_partition_info(const path &p) noexcept;

    // Partition enumeration
    [[nodiscard]] FRAPPE_API result<std::vector<partition_info>> list_partitions() noexcept;
    [[nodiscard]] FRAPPE_API result<std::vector<partition_info>> list_mounted_partitions() noexcept;
    [[nodiscard]] FRAPPE_API result<std::optional<partition_info>>
    get_partition_by_uuid(std::string_view uuid) noexcept;
    [[nodiscard]] FRAPPE_API result<std::optional<partition_info>>
    get_partition_by_label(std::string_view label) noexcept;

    // Free regions
    [[nodiscard]] FRAPPE_API result<std::vector<free_region>> get_free_regions(std::string_view device_path) noexcept;
    [[nodiscard]] FRAPPE_API result<std::uint64_t> get_total_free_space(std::string_view device_path) noexcept;

    // Utility
    [[nodiscard]] FRAPPE_API result<std::string> get_containing_device(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_system_partition(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_boot_partition(const path &p) noexcept;

    // ============================================================================
    // 7.12 Mount Information
    // ============================================================================

    struct mount_info {
        std::string device;      // Device path
        std::string mount_point; // Mount point path
        filesystem_type fs_type = filesystem_type::unknown;
        std::string fs_type_name; // Raw filesystem type string
        std::vector<std::string> options;
        bool is_readonly = false;
        bool is_network = false;
        bool is_removable = false;
    };

    // Mount enumeration
    [[nodiscard]] FRAPPE_API result<std::vector<mount_info>> list_mounts() noexcept;
    [[nodiscard]] FRAPPE_API result<mount_info> get_mount_info(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_mounted(std::string_view device) noexcept;
    [[nodiscard]] FRAPPE_API result<std::string> get_mount_point(std::string_view device) noexcept;
    [[nodiscard]] FRAPPE_API result<std::string> get_device_by_mount_point(const path &mount_point) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_mount_point(const path &p) noexcept;

    // Additional disk queries
    [[nodiscard]] FRAPPE_API result<std::vector<partition_info>> list_unmounted_partitions() noexcept;
    [[nodiscard]] FRAPPE_API result<disk_info> get_boot_disk() noexcept;
    [[nodiscard]] FRAPPE_API result<disk_info> get_system_disk() noexcept;

    // ============================================================================
    // 7.8 Extended Disk Query Functions
    // ============================================================================

    [[nodiscard]] FRAPPE_API result<std::vector<disk_info>> list_physical_disks() noexcept;
    [[nodiscard]] FRAPPE_API result<std::vector<disk_info>> list_removable_disks() noexcept;
    [[nodiscard]] FRAPPE_API result<std::optional<disk_info>> get_disk_by_serial(std::string_view serial) noexcept;

    // ============================================================================
    // 7.9 S.M.A.R.T. Information
    // ============================================================================

    enum class smart_status { unknown, good, warning, critical, failing };

    [[nodiscard]] FRAPPE_API std::string_view smart_status_name(smart_status status) noexcept;

    struct smart_attribute {
        std::uint8_t id = 0;
        std::string name;
        std::uint8_t current = 0;
        std::uint8_t worst = 0;
        std::uint8_t threshold = 0;
        std::uint64_t raw_value = 0;
        smart_status status = smart_status::unknown;
    };

    struct smart_info {
        bool is_supported = false;
        bool is_enabled = false;
        smart_status health_status = smart_status::unknown;

        std::optional<std::int32_t> temperature; // Celsius
        std::optional<std::uint64_t> power_on_hours;
        std::optional<std::uint64_t> power_cycle_count;
        std::optional<std::uint64_t> reallocated_sectors;
        std::optional<std::uint64_t> pending_sectors;
        std::optional<std::uint64_t> uncorrectable_errors;

        std::vector<smart_attribute> attributes;
    };

    [[nodiscard]] FRAPPE_API result<smart_info> get_smart_info(std::string_view device_path) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_smart_supported(std::string_view device_path) noexcept;
    [[nodiscard]] FRAPPE_API result<smart_status> get_disk_health(std::string_view device_path) noexcept;
    [[nodiscard]] FRAPPE_API result<std::optional<std::int32_t>>
    get_disk_temperature(std::string_view device_path) noexcept;

    // ============================================================================
    // 7.10 NVMe Information
    // ============================================================================

    struct nvme_info {
        std::uint16_t controller_id = 0;
        std::uint32_t namespace_id = 0;
        std::string pci_address;
        std::uint8_t firmware_slot = 0;

        std::uint8_t available_spare = 0; // Percentage
        std::uint8_t available_spare_threshold = 0;
        std::uint8_t percentage_used = 0;

        std::uint64_t data_units_read = 0; // In 512KB units
        std::uint64_t data_units_written = 0;
        std::uint64_t host_read_commands = 0;
        std::uint64_t host_write_commands = 0;
        std::uint64_t controller_busy_time = 0; // Minutes
        std::uint64_t power_cycles = 0;
        std::uint64_t power_on_hours = 0;
        std::uint64_t unsafe_shutdowns = 0;
        std::uint64_t media_errors = 0;
        std::uint64_t error_log_entries = 0;

        std::optional<std::int32_t> temperature; // Celsius
        std::optional<std::int32_t> warning_temp_threshold;
        std::optional<std::int32_t> critical_temp_threshold;
    };

    [[nodiscard]] FRAPPE_API result<nvme_info> get_nvme_info(std::string_view device_path) noexcept;
    [[nodiscard]] FRAPPE_API result<bool> is_nvme_device(std::string_view device_path) noexcept;

    // ============================================================================
    // 7.11 Storage Pool / RAID Information
    // ============================================================================

    enum class storage_pool_type {
        unknown,
        lvm,            // Linux LVM
        zfs,            // ZFS pool
        btrfs,          // Btrfs RAID
        mdraid,         // Linux MD RAID
        storage_spaces, // Windows Storage Spaces
        apfs_container  // macOS APFS Container
    };

    enum class raid_level {
        unknown,
        raid0,  // Striping
        raid1,  // Mirroring
        raid5,  // Striping with parity
        raid6,  // Striping with double parity
        raid10, // Mirrored stripes
        raid01, // Striped mirrors
        jbod,   // Just a Bunch Of Disks
        single  // Single device
    };

    enum class raid_state { unknown, active, degraded, rebuilding, resyncing, failed, inactive };

    [[nodiscard]] FRAPPE_API std::string_view storage_pool_type_name(storage_pool_type type) noexcept;
    [[nodiscard]] FRAPPE_API std::string_view raid_level_name(raid_level level) noexcept;
    [[nodiscard]] FRAPPE_API std::string_view raid_state_name(raid_state state) noexcept;

    struct storage_pool_info {
        std::string name;
        storage_pool_type type = storage_pool_type::unknown;
        std::uint64_t size = 0;
        std::uint64_t used = 0;
        std::uint64_t free = 0;
        smart_status health = smart_status::unknown;
        std::vector<std::string> members; // Member device paths
    };

    struct raid_info {
        std::string name;
        std::string device_path; // e.g., /dev/md0
        raid_level level = raid_level::unknown;
        raid_state state = raid_state::unknown;
        std::uint64_t size = 0;
        std::uint32_t chunk_size = 0; // Stripe size in bytes

        std::vector<std::string> devices;
        std::vector<std::string> spare_devices;
        std::vector<std::string> failed_devices;

        std::optional<double> sync_progress;     // 0.0 - 1.0
        std::optional<std::uint64_t> sync_speed; // Bytes per second
    };

    [[nodiscard]] FRAPPE_API result<std::vector<storage_pool_info>> list_storage_pools() noexcept;
    [[nodiscard]] FRAPPE_API result<std::vector<raid_info>> list_raid_arrays() noexcept;
    [[nodiscard]] FRAPPE_API result<std::optional<storage_pool_info>> get_storage_pool(std::string_view name) noexcept;
    [[nodiscard]] FRAPPE_API result<std::optional<raid_info>> get_raid_info(std::string_view device_path) noexcept;

    // ============================================================================
    // 7.13 Virtual Disk Support
    // ============================================================================

    enum class virtual_disk_type {
        unknown,
        vhd,    // Virtual Hard Disk (Microsoft)
        vhdx,   // Hyper-V Virtual Hard Disk
        vmdk,   // VMware Virtual Machine Disk
        vdi,    // VirtualBox Disk Image
        qcow,   // QEMU Copy-On-Write
        qcow2,  // QEMU Copy-On-Write v2
        raw,    // Raw disk image
        iso,    // ISO 9660 image
        dmg,    // macOS Disk Image
        sparse, // Sparse file image
        img     // Generic disk image
    };

    [[nodiscard]] FRAPPE_API std::string_view virtual_disk_type_name(virtual_disk_type type) noexcept;

    struct virtual_disk_info {
        path file_path;
        virtual_disk_type type = virtual_disk_type::unknown;

        std::uint64_t virtual_size = 0; // Allocated/max size
        std::uint64_t actual_size = 0;  // Actual file size
        std::uint32_t block_size = 0;

        bool is_dynamic = false;         // Dynamically expanding
        bool is_differencing = false;    // Differencing/snapshot disk
        std::optional<path> parent_path; // Parent disk for differencing

        bool is_mounted = false;
        std::optional<std::string> mount_point;

        bool is_encrypted = false;
        std::optional<std::string> compression; // Compression type if any

        std::optional<std::string> uuid;
        std::optional<std::filesystem::file_time_type> creation_time;
        std::optional<std::filesystem::file_time_type> modification_time;
    };

    [[nodiscard]] FRAPPE_API result<bool> is_virtual_disk(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<virtual_disk_type> get_virtual_disk_type(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<virtual_disk_info> get_virtual_disk_info(const path &p) noexcept;
    [[nodiscard]] FRAPPE_API result<std::vector<virtual_disk_info>> list_mounted_virtual_disks() noexcept;
    [[nodiscard]] FRAPPE_API result<std::vector<path>> list_virtual_disk_files(const path &directory) noexcept;
    [[nodiscard]] FRAPPE_API result<std::vector<path>> get_virtual_disk_chain(const path &p) noexcept;

} // namespace frappe

#endif // FRAPPE_DISK_HPP
