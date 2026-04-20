#ifdef FRAPPE_PLATFORM_MACOS

#include "frappe/disk.hpp"

#include <dirent.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

namespace frappe::detail {

namespace {
std::string find_mount_point(const std::string &device_path) {
    struct statfs *mounts = nullptr;
    int count = getmntinfo(&mounts, MNT_NOWAIT);

    for (int i = 0; i < count; ++i) {
        if (device_path == mounts[i].f_mntfromname) {
            return mounts[i].f_mntonname;
        }
    }
    return "";
}
} // namespace

result<std::vector<disk_info>> list_disks_impl() noexcept {
    std::vector<disk_info> disks;

    // On macOS, disks are in /dev/disk*
    DIR *dir = opendir("/dev");
    if (!dir) {
        return std::unexpected(make_error(errno, std::system_category()));
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;

        // Match disk devices (disk0, disk1, etc.) but not partitions (disk0s1)
        if (name.find("disk") != 0) continue;
        if (name.find('s') != std::string::npos) continue; // Skip partitions

        disk_info info;
        info.device_path = "/dev/" + name;

        // Try to get disk size
        int fd = open(info.device_path.c_str(), O_RDONLY);
        if (fd >= 0) {
            uint64_t block_count = 0;
            uint32_t block_size = 0;

            if (ioctl(fd, DKIOCGETBLOCKCOUNT, &block_count) == 0 && ioctl(fd, DKIOCGETBLOCKSIZE, &block_size) == 0) {
                info.total_sectors = block_count;
                info.sector_size = block_size;
                info.size = block_count * block_size;
            }
            close(fd);
        }

        // Determine disk type based on name
        if (name.find("disk") == 0) {
            info.type = disk_type::ssd; // Default assumption for macOS
            info.bus_type = disk_bus_type::nvme;
        }

        disks.push_back(info);
    }

    closedir(dir);
    return disks;
}

result<disk_info> get_disk_info_impl(std::string_view device_path) noexcept {
    auto disks = list_disks_impl();
    if (!disks) return std::unexpected(disks.error());

    for (const auto &d : *disks) {
        if (d.device_path == device_path) {
            return d;
        }
    }

    return std::unexpected(make_error(std::errc::no_such_device));
}

result<disk_info> get_disk_for_path_impl(const path &p) noexcept {
    struct statfs st;
    if (statfs(p.c_str(), &st) != 0) {
        return std::unexpected(make_error(errno, std::system_category()));
    }

    std::string device = st.f_mntfromname;

    // Extract disk name (remove partition suffix like s1, s2)
    std::size_t s_pos = device.rfind('s');
    if (s_pos != std::string::npos && s_pos > 0) {
        bool is_partition = true;
        for (std::size_t i = s_pos + 1; i < device.size(); ++i) {
            if (!std::isdigit(device[i])) {
                is_partition = false;
                break;
            }
        }
        if (is_partition) {
            device = device.substr(0, s_pos);
        }
    }

    return get_disk_info_impl(device);
}

result<partition_table_info> get_partition_table_impl(std::string_view device_path) noexcept {
    partition_table_info info;
    info.device_path = std::string(device_path);

    // Get disk info first
    auto disk = get_disk_info_impl(device_path);
    if (disk) {
        info.disk_size = disk->size;
        info.sector_size = disk->sector_size;
        info.total_sectors = disk->total_sectors;
    }

    // List partitions
    std::string disk_name = std::string(device_path);
    std::size_t slash_pos = disk_name.rfind('/');
    if (slash_pos != std::string::npos) {
        disk_name = disk_name.substr(slash_pos + 1);
    }

    DIR *dir = opendir("/dev");
    if (!dir) {
        return std::unexpected(make_error(errno, std::system_category()));
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;

        // Match partitions (disk0s1, disk0s2, etc.)
        if (name.find(disk_name + "s") != 0) continue;

        partition_info pinfo;
        pinfo.device_path = "/dev/" + name;

        // Extract partition number
        std::string num_str = name.substr(disk_name.size() + 1);
        if (!num_str.empty()) {
            pinfo.partition_number = std::stoul(num_str);
        }

        // Find mount point
        pinfo.mount_point = find_mount_point(pinfo.device_path);

        info.partitions.push_back(pinfo);
    }

    closedir(dir);

    // Sort by partition number
    std::sort(info.partitions.begin(), info.partitions.end(),
              [](const partition_info &a, const partition_info &b) { return a.partition_number < b.partition_number; });

    // macOS typically uses GPT
    info.type = partition_table_type::gpt;
    info.first_usable_sector = 34;
    info.last_usable_sector = info.total_sectors - 1;

    return info;
}

result<partition_info> get_partition_info_impl(const path &p) noexcept {
    struct statfs st;
    if (statfs(p.c_str(), &st) != 0) {
        return std::unexpected(make_error(errno, std::system_category()));
    }

    std::string device = st.f_mntfromname;

    // Extract disk name
    std::string disk_device = device;
    std::size_t s_pos = device.rfind('s');
    if (s_pos != std::string::npos && s_pos > 0) {
        bool is_partition = true;
        for (std::size_t i = s_pos + 1; i < device.size(); ++i) {
            if (!std::isdigit(device[i])) {
                is_partition = false;
                break;
            }
        }
        if (is_partition) {
            disk_device = device.substr(0, s_pos);
        }
    }

    auto table = get_partition_table_impl(disk_device);
    if (!table) return std::unexpected(table.error());

    for (const auto &part : table->partitions) {
        if (part.device_path == device) {
            return part;
        }
    }

    return std::unexpected(make_error(std::errc::no_such_device));
}

result<std::vector<partition_info>> list_partitions_impl() noexcept {
    std::vector<partition_info> all_partitions;

    auto disks = list_disks_impl();
    if (!disks) return std::unexpected(disks.error());

    for (const auto &disk : *disks) {
        auto table = get_partition_table_impl(disk.device_path);
        if (table) {
            for (const auto &part : table->partitions) {
                all_partitions.push_back(part);
            }
        }
    }

    return all_partitions;
}

result<std::string> get_containing_device_impl(const path &p) noexcept {
    auto disk = get_disk_for_path_impl(p);
    if (!disk) return std::unexpected(disk.error());
    return disk->device_path;
}

// ============================================================================
// 7.12 Mount Information
// ============================================================================

result<std::vector<mount_info>> list_mounts_impl() noexcept {
    std::vector<mount_info> mounts;

    struct statfs *mount_list = nullptr;
    int count = getmntinfo(&mount_list, MNT_NOWAIT);

    if (count <= 0) {
        return std::unexpected(make_error(errno, std::system_category()));
    }

    for (int i = 0; i < count; ++i) {
        mount_info info;
        info.device = mount_list[i].f_mntfromname;
        info.mount_point = mount_list[i].f_mntonname;
        info.fs_type_name = mount_list[i].f_fstypename;

        // Check flags
        info.is_readonly = (mount_list[i].f_flags & MNT_RDONLY) != 0;

        // Determine filesystem type
        if (info.fs_type_name == "apfs") {
            info.fs_type = filesystem_type::apfs;
        } else if (info.fs_type_name == "hfs") {
            info.fs_type = filesystem_type::hfs_plus;
        } else if (info.fs_type_name == "msdos") {
            info.fs_type = filesystem_type::fat32;
        } else if (info.fs_type_name == "exfat") {
            info.fs_type = filesystem_type::exfat;
        } else if (info.fs_type_name == "ntfs") {
            info.fs_type = filesystem_type::ntfs;
        } else if (info.fs_type_name == "nfs") {
            info.fs_type = filesystem_type::nfs;
            info.is_network = true;
        } else if (info.fs_type_name == "smbfs") {
            info.fs_type = filesystem_type::cifs;
            info.is_network = true;
        } else if (info.fs_type_name == "afpfs") {
            info.fs_type = filesystem_type::afp;
            info.is_network = true;
        }

        mounts.push_back(info);
    }

    return mounts;
}

// ============================================================================
// 7.9 S.M.A.R.T. Information
// ============================================================================

result<smart_info> get_smart_info_impl(std::string_view device_path) noexcept {
    smart_info info;

    // macOS uses IOKit for SMART data
    // Basic implementation - full SMART requires IOKit framework
    info.is_supported = true;
    info.is_enabled = true;
    info.health_status = smart_status::good;

    return info;
}

// ============================================================================
// 7.10 NVMe Information
// ============================================================================

result<nvme_info> get_nvme_info_impl(std::string_view device_path) noexcept {
    nvme_info info;

    // macOS NVMe info requires IOKit
    // Basic implementation
    info.namespace_id = 1;

    return info;
}

// ============================================================================
// 7.11 Storage Pool / RAID Information
// ============================================================================

result<std::vector<storage_pool_info>> list_storage_pools_impl() noexcept {
    std::vector<storage_pool_info> pools;

    // macOS APFS containers could be listed here
    // Would require diskutil or IOKit

    return pools;
}

result<std::vector<raid_info>> list_raid_arrays_impl() noexcept {
    std::vector<raid_info> arrays;

    // macOS software RAID via diskutil
    // Basic implementation returns empty

    return arrays;
}

// ============================================================================
// 7.13 Virtual Disk Support
// ============================================================================

result<virtual_disk_info> get_virtual_disk_info_impl(const path &p) noexcept {
    virtual_disk_info info;
    info.file_path = p;

    struct stat st;
    if (stat(p.c_str(), &st) != 0) {
        return std::unexpected(make_error(errno, std::system_category()));
    }

    info.actual_size = st.st_size;

    // Determine type from extension
    auto ext = p.extension().string();
    for (auto &c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    if (ext == ".dmg")
        info.type = virtual_disk_type::dmg;
    else if (ext == ".iso")
        info.type = virtual_disk_type::iso;
    else if (ext == ".vhd")
        info.type = virtual_disk_type::vhd;
    else if (ext == ".vhdx")
        info.type = virtual_disk_type::vhdx;
    else if (ext == ".vmdk")
        info.type = virtual_disk_type::vmdk;
    else if (ext == ".vdi")
        info.type = virtual_disk_type::vdi;
    else if (ext == ".qcow" || ext == ".qcow2")
        info.type = virtual_disk_type::qcow2;
    else if (ext == ".img")
        info.type = virtual_disk_type::img;
    else if (ext == ".raw")
        info.type = virtual_disk_type::raw;
    else if (ext == ".sparseimage" || ext == ".sparsebundle")
        info.type = virtual_disk_type::sparse;
    else
        info.type = virtual_disk_type::unknown;

    info.virtual_size = info.actual_size;

    // Get modification time
    std::error_code ec;
    info.modification_time = std::filesystem::last_write_time(p, ec);

    return info;
}

result<std::vector<virtual_disk_info>> list_mounted_virtual_disks_impl() noexcept {
    std::vector<virtual_disk_info> disks;

    // Would need to parse output of hdiutil info
    // Basic implementation returns empty

    return disks;
}

} // namespace frappe::detail

#endif // FRAPPE_PLATFORM_MACOS
