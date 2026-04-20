#include "frappe/disk.hpp"

namespace frappe {

std::string_view partition_table_type_name(partition_table_type type) noexcept {
    switch (type) {
    case partition_table_type::mbr:
        return "mbr";
    case partition_table_type::gpt:
        return "gpt";
    case partition_table_type::apm:
        return "apm";
    case partition_table_type::bsd:
        return "bsd";
    case partition_table_type::sun:
        return "sun";
    default:
        return "unknown";
    }
}

std::string_view partition_type_name(partition_type type) noexcept {
    switch (type) {
    case partition_type::efi_system:
        return "efi_system";
    case partition_type::bios_boot:
        return "bios_boot";
    case partition_type::microsoft_reserved:
        return "microsoft_reserved";
    case partition_type::microsoft_basic_data:
        return "microsoft_basic_data";
    case partition_type::microsoft_ldm_metadata:
        return "microsoft_ldm_metadata";
    case partition_type::microsoft_ldm_data:
        return "microsoft_ldm_data";
    case partition_type::microsoft_recovery:
        return "microsoft_recovery";
    case partition_type::microsoft_storage_spaces:
        return "microsoft_storage_spaces";
    case partition_type::linux_filesystem:
        return "linux_filesystem";
    case partition_type::linux_swap:
        return "linux_swap";
    case partition_type::linux_lvm:
        return "linux_lvm";
    case partition_type::linux_raid:
        return "linux_raid";
    case partition_type::linux_home:
        return "linux_home";
    case partition_type::linux_root_x86:
        return "linux_root_x86";
    case partition_type::linux_root_x86_64:
        return "linux_root_x86_64";
    case partition_type::linux_root_arm:
        return "linux_root_arm";
    case partition_type::linux_root_arm64:
        return "linux_root_arm64";
    case partition_type::apple_hfs:
        return "apple_hfs";
    case partition_type::apple_apfs:
        return "apple_apfs";
    case partition_type::apple_boot:
        return "apple_boot";
    case partition_type::apple_raid:
        return "apple_raid";
    case partition_type::apple_label:
        return "apple_label";
    case partition_type::freebsd_boot:
        return "freebsd_boot";
    case partition_type::freebsd_data:
        return "freebsd_data";
    case partition_type::freebsd_swap:
        return "freebsd_swap";
    case partition_type::solaris_root:
        return "solaris_root";
    case partition_type::solaris_swap:
        return "solaris_swap";
    default:
        return "unknown";
    }
}

std::string_view disk_type_name(disk_type type) noexcept {
    switch (type) {
    case disk_type::hdd:
        return "hdd";
    case disk_type::ssd:
        return "ssd";
    case disk_type::nvme:
        return "nvme";
    case disk_type::usb:
        return "usb";
    case disk_type::optical:
        return "optical";
    case disk_type::floppy:
        return "floppy";
    case disk_type::virtual_disk:
        return "virtual_disk";
    case disk_type::ram_disk:
        return "ram_disk";
    case disk_type::network:
        return "network";
    default:
        return "unknown";
    }
}

std::string_view disk_bus_type_name(disk_bus_type type) noexcept {
    switch (type) {
    case disk_bus_type::ata:
        return "ata";
    case disk_bus_type::sata:
        return "sata";
    case disk_bus_type::scsi:
        return "scsi";
    case disk_bus_type::sas:
        return "sas";
    case disk_bus_type::usb:
        return "usb";
    case disk_bus_type::nvme:
        return "nvme";
    case disk_bus_type::pcie:
        return "pcie";
    case disk_bus_type::thunderbolt:
        return "thunderbolt";
    case disk_bus_type::firewire:
        return "firewire";
    case disk_bus_type::virtual_bus:
        return "virtual";
    case disk_bus_type::mmc:
        return "mmc";
    case disk_bus_type::raid:
        return "raid";
    default:
        return "unknown";
    }
}

namespace detail {
result<std::vector<disk_info>> list_disks_impl() noexcept;
result<disk_info> get_disk_info_impl(std::string_view device_path) noexcept;
result<disk_info> get_disk_for_path_impl(const path &p) noexcept;
result<partition_table_info> get_partition_table_impl(std::string_view device_path) noexcept;
result<partition_info> get_partition_info_impl(const path &p) noexcept;
result<std::vector<partition_info>> list_partitions_impl() noexcept;
result<std::string> get_containing_device_impl(const path &p) noexcept;
} // namespace detail

result<std::vector<disk_info>> list_disks() noexcept {
    return detail::list_disks_impl();
}

result<disk_info> get_disk_info(std::string_view device_path) noexcept {
    return detail::get_disk_info_impl(device_path);
}

result<disk_info> get_disk_for_path(const path &p) noexcept {
    return detail::get_disk_for_path_impl(p);
}

result<partition_table_info> get_partition_table(std::string_view device_path) noexcept {
    return detail::get_partition_table_impl(device_path);
}

result<partition_info> get_partition_info(const path &p) noexcept {
    return detail::get_partition_info_impl(p);
}

result<std::vector<partition_info>> list_partitions() noexcept {
    return detail::list_partitions_impl();
}

result<std::vector<partition_info>> list_mounted_partitions() noexcept {
    auto all = list_partitions();
    if (!all) return std::unexpected(all.error());

    std::vector<partition_info> mounted;
    for (const auto &p : *all) {
        if (!p.mount_point.empty()) {
            mounted.push_back(p);
        }
    }
    return mounted;
}

result<std::optional<partition_info>> get_partition_by_uuid(std::string_view uuid) noexcept {
    auto all = list_partitions();
    if (!all) return std::unexpected(all.error());

    for (const auto &p : *all) {
        if (p.uuid == uuid) {
            return p;
        }
    }
    return std::nullopt;
}

result<std::optional<partition_info>> get_partition_by_label(std::string_view label) noexcept {
    auto all = list_partitions();
    if (!all) return std::unexpected(all.error());

    for (const auto &p : *all) {
        if (p.label == label) {
            return p;
        }
    }
    return std::nullopt;
}

result<std::vector<free_region>> get_free_regions(std::string_view device_path) noexcept {
    auto table = get_partition_table(device_path);
    if (!table) return std::unexpected(table.error());

    std::vector<free_region> regions;
    std::uint64_t current = table->first_usable_sector;

    // Sort partitions by start sector
    auto partitions = table->partitions;
    std::sort(partitions.begin(), partitions.end(),
              [](const partition_info &a, const partition_info &b) { return a.start_sector < b.start_sector; });

    for (const auto &p : partitions) {
        if (p.start_sector > current) {
            free_region region;
            region.start_sector = current;
            region.end_sector = p.start_sector - 1;
            region.size = (region.end_sector - region.start_sector + 1) * table->sector_size;
            region.is_primary_slot = true;
            regions.push_back(region);
        }
        current = p.end_sector + 1;
    }

    // Check for free space at end
    if (current < table->last_usable_sector) {
        free_region region;
        region.start_sector = current;
        region.end_sector = table->last_usable_sector;
        region.size = (region.end_sector - region.start_sector + 1) * table->sector_size;
        region.is_primary_slot = true;
        regions.push_back(region);
    }

    return regions;
}

result<std::uint64_t> get_total_free_space(std::string_view device_path) noexcept {
    auto regions = get_free_regions(device_path);
    if (!regions) return std::unexpected(regions.error());

    std::uint64_t total = 0;
    for (const auto &r : *regions) {
        total += r.size;
    }
    return total;
}

result<std::string> get_containing_device(const path &p) noexcept {
    return detail::get_containing_device_impl(p);
}

result<bool> is_system_partition(const path &p) noexcept {
    auto info = get_partition_info(p);
    if (!info) return std::unexpected(info.error());

    // Check for system partition types
    return info->type == partition_type::efi_system || info->type == partition_type::microsoft_reserved ||
           info->type == partition_type::microsoft_recovery;
}

result<bool> is_boot_partition(const path &p) noexcept {
    auto info = get_partition_info(p);
    if (!info) return std::unexpected(info.error());

    return info->is_bootable || info->is_active || info->type == partition_type::efi_system ||
           info->type == partition_type::bios_boot;
}

// ============================================================================
// 7.12 Mount Information
// ============================================================================

namespace detail {
result<std::vector<mount_info>> list_mounts_impl() noexcept;
}

result<std::vector<mount_info>> list_mounts() noexcept {
    return detail::list_mounts_impl();
}

result<mount_info> get_mount_info(const path &p) noexcept {
    auto mounts = list_mounts();
    if (!mounts) return std::unexpected(mounts.error());

    std::string path_str = p.string();
    mount_info best_match;
    std::size_t best_len = 0;

    for (const auto &m : *mounts) {
        if (path_str.find(m.mount_point) == 0 && m.mount_point.size() > best_len) {
            best_match = m;
            best_len = m.mount_point.size();
        }
    }

    if (best_len == 0) {
        return std::unexpected(make_error(std::errc::no_such_device));
    }

    return best_match;
}

result<bool> is_mounted(std::string_view device) noexcept {
    auto mounts = list_mounts();
    if (!mounts) return std::unexpected(mounts.error());

    for (const auto &m : *mounts) {
        if (m.device == device) {
            return true;
        }
    }
    return false;
}

result<std::string> get_mount_point(std::string_view device) noexcept {
    auto mounts = list_mounts();
    if (!mounts) return std::unexpected(mounts.error());

    for (const auto &m : *mounts) {
        if (m.device == device) {
            return m.mount_point;
        }
    }
    return std::unexpected(make_error(std::errc::no_such_device));
}

result<std::string> get_device_by_mount_point(const path &mount_point) noexcept {
    auto mounts = list_mounts();
    if (!mounts) return std::unexpected(mounts.error());

    std::string mp_str = mount_point.string();
    for (const auto &m : *mounts) {
        if (m.mount_point == mp_str) {
            return m.device;
        }
    }
    return std::unexpected(make_error(std::errc::no_such_device));
}

result<bool> is_mount_point(const path &p) noexcept {
    auto mounts = list_mounts();
    if (!mounts) return std::unexpected(mounts.error());

    std::string path_str = p.string();
    for (const auto &m : *mounts) {
        if (m.mount_point == path_str) {
            return true;
        }
    }
    return false;
}

result<std::vector<partition_info>> list_unmounted_partitions() noexcept {
    auto all = list_partitions();
    if (!all) return std::unexpected(all.error());

    std::vector<partition_info> unmounted;
    for (const auto &p : *all) {
        if (p.mount_point.empty()) {
            unmounted.push_back(p);
        }
    }
    return unmounted;
}

result<disk_info> get_boot_disk() noexcept {
#ifdef _WIN32
    return get_disk_for_path("C:\\");
#else
    return get_disk_for_path("/boot");
#endif
}

result<disk_info> get_system_disk() noexcept {
#ifdef _WIN32
    return get_disk_for_path("C:\\Windows");
#else
    return get_disk_for_path("/");
#endif
}

// ============================================================================
// 7.8 Extended Disk Query Functions
// ============================================================================

result<std::vector<disk_info>> list_physical_disks() noexcept {
    auto all = list_disks();
    if (!all) return std::unexpected(all.error());

    std::vector<disk_info> physical;
    for (const auto &d : *all) {
        if (d.type != disk_type::virtual_disk && d.type != disk_type::ram_disk) {
            physical.push_back(d);
        }
    }
    return physical;
}

result<std::vector<disk_info>> list_removable_disks() noexcept {
    auto all = list_disks();
    if (!all) return std::unexpected(all.error());

    std::vector<disk_info> removable;
    for (const auto &d : *all) {
        if (d.is_removable) {
            removable.push_back(d);
        }
    }
    return removable;
}

result<std::optional<disk_info>> get_disk_by_serial(std::string_view serial) noexcept {
    auto all = list_disks();
    if (!all) return std::unexpected(all.error());

    for (const auto &d : *all) {
        if (d.serial == serial) {
            return d;
        }
    }
    return std::nullopt;
}

// ============================================================================
// 7.9 S.M.A.R.T. Information
// ============================================================================

std::string_view smart_status_name(smart_status status) noexcept {
    switch (status) {
    case smart_status::good:
        return "good";
    case smart_status::warning:
        return "warning";
    case smart_status::critical:
        return "critical";
    case smart_status::failing:
        return "failing";
    default:
        return "unknown";
    }
}

namespace detail {
result<smart_info> get_smart_info_impl(std::string_view device_path) noexcept;
result<nvme_info> get_nvme_info_impl(std::string_view device_path) noexcept;
result<std::vector<storage_pool_info>> list_storage_pools_impl() noexcept;
result<std::vector<raid_info>> list_raid_arrays_impl() noexcept;
} // namespace detail

result<smart_info> get_smart_info(std::string_view device_path) noexcept {
    return detail::get_smart_info_impl(device_path);
}

result<bool> is_smart_supported(std::string_view device_path) noexcept {
    auto info = get_smart_info(device_path);
    if (!info) return std::unexpected(info.error());
    return info->is_supported;
}

result<smart_status> get_disk_health(std::string_view device_path) noexcept {
    auto info = get_smart_info(device_path);
    if (!info) return std::unexpected(info.error());
    return info->health_status;
}

result<std::optional<std::int32_t>> get_disk_temperature(std::string_view device_path) noexcept {
    auto info = get_smart_info(device_path);
    if (!info) return std::unexpected(info.error());
    return info->temperature;
}

// ============================================================================
// 7.10 NVMe Information
// ============================================================================

result<nvme_info> get_nvme_info(std::string_view device_path) noexcept {
    return detail::get_nvme_info_impl(device_path);
}

result<bool> is_nvme_device(std::string_view device_path) noexcept {
    auto disk = get_disk_info(device_path);
    if (!disk) return std::unexpected(disk.error());
    return disk->type == disk_type::nvme || disk->bus_type == disk_bus_type::nvme;
}

// ============================================================================
// 7.11 Storage Pool / RAID Information
// ============================================================================

std::string_view storage_pool_type_name(storage_pool_type type) noexcept {
    switch (type) {
    case storage_pool_type::lvm:
        return "lvm";
    case storage_pool_type::zfs:
        return "zfs";
    case storage_pool_type::btrfs:
        return "btrfs";
    case storage_pool_type::mdraid:
        return "mdraid";
    case storage_pool_type::storage_spaces:
        return "storage_spaces";
    case storage_pool_type::apfs_container:
        return "apfs_container";
    default:
        return "unknown";
    }
}

std::string_view raid_level_name(raid_level level) noexcept {
    switch (level) {
    case raid_level::raid0:
        return "raid0";
    case raid_level::raid1:
        return "raid1";
    case raid_level::raid5:
        return "raid5";
    case raid_level::raid6:
        return "raid6";
    case raid_level::raid10:
        return "raid10";
    case raid_level::raid01:
        return "raid01";
    case raid_level::jbod:
        return "jbod";
    case raid_level::single:
        return "single";
    default:
        return "unknown";
    }
}

std::string_view raid_state_name(raid_state state) noexcept {
    switch (state) {
    case raid_state::active:
        return "active";
    case raid_state::degraded:
        return "degraded";
    case raid_state::rebuilding:
        return "rebuilding";
    case raid_state::resyncing:
        return "resyncing";
    case raid_state::failed:
        return "failed";
    case raid_state::inactive:
        return "inactive";
    default:
        return "unknown";
    }
}

result<std::vector<storage_pool_info>> list_storage_pools() noexcept {
    return detail::list_storage_pools_impl();
}

result<std::vector<raid_info>> list_raid_arrays() noexcept {
    return detail::list_raid_arrays_impl();
}

result<std::optional<storage_pool_info>> get_storage_pool(std::string_view name) noexcept {
    auto pools = list_storage_pools();
    if (!pools) return std::unexpected(pools.error());

    for (const auto &p : *pools) {
        if (p.name == name) {
            return p;
        }
    }
    return std::nullopt;
}

result<std::optional<raid_info>> get_raid_info(std::string_view device_path) noexcept {
    auto arrays = list_raid_arrays();
    if (!arrays) return std::unexpected(arrays.error());

    for (const auto &r : *arrays) {
        if (r.device_path == device_path) {
            return r;
        }
    }
    return std::nullopt;
}

// ============================================================================
// 7.13 Virtual Disk Support
// ============================================================================

std::string_view virtual_disk_type_name(virtual_disk_type type) noexcept {
    switch (type) {
    case virtual_disk_type::vhd:
        return "vhd";
    case virtual_disk_type::vhdx:
        return "vhdx";
    case virtual_disk_type::vmdk:
        return "vmdk";
    case virtual_disk_type::vdi:
        return "vdi";
    case virtual_disk_type::qcow:
        return "qcow";
    case virtual_disk_type::qcow2:
        return "qcow2";
    case virtual_disk_type::raw:
        return "raw";
    case virtual_disk_type::iso:
        return "iso";
    case virtual_disk_type::dmg:
        return "dmg";
    case virtual_disk_type::sparse:
        return "sparse";
    case virtual_disk_type::img:
        return "img";
    default:
        return "unknown";
    }
}

namespace detail {
result<virtual_disk_info> get_virtual_disk_info_impl(const path &p) noexcept;
result<std::vector<virtual_disk_info>> list_mounted_virtual_disks_impl() noexcept;
} // namespace detail

result<bool> is_virtual_disk(const path &p) noexcept {
    auto ext = p.extension().string();
    // Convert to lowercase
    for (auto &c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    if (ext == ".vhd" || ext == ".vhdx" || ext == ".vmdk" || ext == ".vdi" || ext == ".qcow" || ext == ".qcow2" ||
        ext == ".iso" || ext == ".dmg" || ext == ".img") {
        return true;
    }
    return false;
}

result<virtual_disk_type> get_virtual_disk_type(const path &p) noexcept {
    auto ext = p.extension().string();
    for (auto &c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    if (ext == ".vhd") return virtual_disk_type::vhd;
    if (ext == ".vhdx") return virtual_disk_type::vhdx;
    if (ext == ".vmdk") return virtual_disk_type::vmdk;
    if (ext == ".vdi") return virtual_disk_type::vdi;
    if (ext == ".qcow") return virtual_disk_type::qcow;
    if (ext == ".qcow2") return virtual_disk_type::qcow2;
    if (ext == ".iso") return virtual_disk_type::iso;
    if (ext == ".dmg") return virtual_disk_type::dmg;
    if (ext == ".img") return virtual_disk_type::img;
    if (ext == ".raw") return virtual_disk_type::raw;

    return virtual_disk_type::unknown;
}

result<virtual_disk_info> get_virtual_disk_info(const path &p) noexcept {
    return detail::get_virtual_disk_info_impl(p);
}

result<std::vector<virtual_disk_info>> list_mounted_virtual_disks() noexcept {
    return detail::list_mounted_virtual_disks_impl();
}

result<std::vector<path>> list_virtual_disk_files(const path &directory) noexcept {
    std::error_code ec;
    if (!std::filesystem::exists(directory, ec) || !std::filesystem::is_directory(directory, ec)) {
        return std::unexpected(make_error(std::errc::not_a_directory));
    }

    std::vector<path> files;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(directory, ec)) {
        if (entry.is_regular_file()) {
            auto is_vd = is_virtual_disk(entry.path());
            if (is_vd && *is_vd) {
                files.push_back(entry.path());
            }
        }
    }
    return files;
}

result<std::vector<path>> get_virtual_disk_chain(const path &p) noexcept {
    auto info = get_virtual_disk_info(p);
    if (!info) return std::unexpected(info.error());

    std::vector<path> chain;
    chain.push_back(p);

    if (info->is_differencing && info->parent_path) {
        auto parent_chain = get_virtual_disk_chain(*info->parent_path);
        if (parent_chain) {
            chain.insert(chain.end(), parent_chain->begin(), parent_chain->end());
        }
    }

    return chain;
}

} // namespace frappe
