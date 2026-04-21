#ifdef FRAPPE_PLATFORM_LINUX

#include "frappe/filesystem.hpp"

#include <cstring>
#include <fstream>
#include <mntent.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

namespace frappe::detail {

namespace {
filesystem_type parse_filesystem_name(const std::string &name) {
    if (name == "ext2") {
        return filesystem_type::ext2;
    }
    if (name == "ext3") {
        return filesystem_type::ext3;
    }
    if (name == "ext4") {
        return filesystem_type::ext4;
    }
    if (name == "xfs") {
        return filesystem_type::xfs;
    }
    if (name == "btrfs") {
        return filesystem_type::btrfs;
    }
    if (name == "zfs") {
        return filesystem_type::zfs;
    }
    if (name == "ntfs" || name == "ntfs-3g" || name == "fuseblk") {
        return filesystem_type::ntfs;
    }
    if (name == "vfat" || name == "fat") {
        return filesystem_type::fat;
    }
    if (name == "fat32") {
        return filesystem_type::fat32;
    }
    if (name == "exfat") {
        return filesystem_type::exfat;
    }
    if (name == "nfs" || name == "nfs4") {
        return filesystem_type::nfs;
    }
    if (name == "cifs" || name == "smbfs") {
        return filesystem_type::smb;
    }
    if (name == "tmpfs") {
        return filesystem_type::tmpfs;
    }
    if (name == "ramfs") {
        return filesystem_type::ramfs;
    }
    if (name == "devtmpfs" || name == "devfs") {
        return filesystem_type::devfs;
    }
    if (name == "proc") {
        return filesystem_type::procfs;
    }
    if (name == "sysfs") {
        return filesystem_type::sysfs;
    }
    if (name.find("fuse") != std::string::npos) {
        return filesystem_type::fuse;
    }
    if (name == "overlay") {
        return filesystem_type::overlay;
    }

    return filesystem_type::unknown;
}

struct mount_entry {
    std::string device;
    std::string mount_point;
    std::string fs_type;
    std::string options;
};

std::vector<mount_entry> get_mount_entries() {
    std::vector<mount_entry> entries;

    FILE *mtab = setmntent("/proc/mounts", "r");
    if (!mtab) {
        mtab = setmntent("/etc/mtab", "r");
    }

    if (mtab) {
        struct mntent *ent;
        while ((ent = getmntent(mtab)) != nullptr) {
            mount_entry entry;
            entry.device = ent->mnt_fsname;
            entry.mount_point = ent->mnt_dir;
            entry.fs_type = ent->mnt_type;
            entry.options = ent->mnt_opts;
            entries.push_back(entry);
        }
        endmntent(mtab);
    }

    return entries;
}

std::optional<mount_entry> find_mount_for_path(const path &p) {
    auto entries = get_mount_entries();
    std::string path_str = std::filesystem::absolute(p).string();

    std::optional<mount_entry> best_match;
    std::size_t best_len = 0;

    for (const auto &entry : entries) {
        if (path_str.find(entry.mount_point) == 0) {
            if (entry.mount_point.size() > best_len) {
                best_len = entry.mount_point.size();
                best_match = entry;
            }
        }
    }

    return best_match;
}
} // namespace

result<filesystem_type> get_filesystem_type_impl(const path &p) noexcept {
    auto mount = find_mount_for_path(p);
    if (!mount) {
        return std::unexpected(make_error(std::errc::no_such_file_or_directory));
    }
    return parse_filesystem_name(mount->fs_type);
}

result<filesystem_features> get_filesystem_features_impl(const path &p) noexcept {
    filesystem_features features;

    auto mount = find_mount_for_path(p);
    if (!mount) {
        return std::unexpected(make_error(std::errc::no_such_file_or_directory));
    }

    features.type = parse_filesystem_name(mount->fs_type);

    struct statvfs st;
    if (statvfs(p.c_str(), &st) == 0) {
        features.block_size = st.f_bsize;
        features.max_filename_length = st.f_namemax;
    }

    // Set features based on filesystem type
    switch (features.type) {
    case filesystem_type::ext4:
    case filesystem_type::ext3:
    case filesystem_type::ext2:
        features.case_sensitive = true;
        features.case_preserving = true;
        features.supports_symlinks = true;
        features.supports_hard_links = true;
        features.supports_permissions = true;
        features.supports_acl = true;
        features.supports_extended_attributes = true;
        features.supports_sparse_files = true;
        features.supports_quotas = true;
        features.max_path_length = 4096;
        break;

    case filesystem_type::xfs:
        features.case_sensitive = true;
        features.case_preserving = true;
        features.supports_symlinks = true;
        features.supports_hard_links = true;
        features.supports_permissions = true;
        features.supports_acl = true;
        features.supports_extended_attributes = true;
        features.supports_sparse_files = true;
        features.supports_quotas = true;
        features.max_path_length = 4096;
        break;

    case filesystem_type::btrfs:
        features.case_sensitive = true;
        features.case_preserving = true;
        features.supports_symlinks = true;
        features.supports_hard_links = true;
        features.supports_permissions = true;
        features.supports_acl = true;
        features.supports_extended_attributes = true;
        features.supports_sparse_files = true;
        features.supports_compression = true;
        features.supports_quotas = true;
        features.max_path_length = 4096;
        break;

    case filesystem_type::ntfs:
        features.case_sensitive = false;
        features.case_preserving = true;
        features.supports_symlinks = true;
        features.supports_hard_links = true;
        features.supports_permissions = true;
        features.supports_acl = true;
        features.supports_extended_attributes = true;
        features.supports_sparse_files = true;
        features.supports_compression = true;
        features.max_path_length = 32767;
        break;

    case filesystem_type::tmpfs:
    case filesystem_type::ramfs:
        features.case_sensitive = true;
        features.case_preserving = true;
        features.supports_symlinks = true;
        features.supports_hard_links = true;
        features.supports_permissions = true;
        break;

    default:
        features.case_sensitive = true;
        features.case_preserving = true;
        features.supports_symlinks = true;
        features.supports_permissions = true;
        break;
    }

    return features;
}

result<volume_info> get_volume_info_impl(const path &p) noexcept {
    volume_info info;

    auto mount = find_mount_for_path(p);
    if (!mount) {
        return std::unexpected(make_error(std::errc::no_such_file_or_directory));
    }

    info.mount_point = mount->mount_point;
    info.device_path = mount->device;
    info.fs_type = parse_filesystem_name(mount->fs_type);
    info.name = mount->mount_point;

    // Get space info
    struct statvfs st;
    if (statvfs(mount->mount_point.c_str(), &st) == 0) {
        info.total_size = static_cast<std::uintmax_t>(st.f_blocks) * st.f_frsize;
        info.free_size = static_cast<std::uintmax_t>(st.f_bfree) * st.f_frsize;
        info.available_size = static_cast<std::uintmax_t>(st.f_bavail) * st.f_frsize;
    }

    // Try to get UUID from /dev/disk/by-uuid
    if (mount->device.find("/dev/") == 0) {
        std::string device_name = mount->device.substr(5);
        std::string uuid_link = "/dev/disk/by-uuid";

        try {
            for (const auto &entry : std::filesystem::directory_iterator(uuid_link)) {
                std::error_code ec;
                auto target = std::filesystem::read_symlink(entry.path(), ec);
                if (!ec) {
                    std::string target_name = target.filename().string();
                    if (target_name == device_name ||
                        ("../../" + target_name) == target.string().substr(target.string().find("../../"))) {
                        info.uuid = entry.path().filename().string();
                        break;
                    }
                }
            }
        } catch (...) {
            // Ignore errors
        }
    }

    return info;
}

result<std::vector<volume_info>> list_volumes_impl() noexcept {
    std::vector<volume_info> volumes;

    auto entries = get_mount_entries();
    for (const auto &entry : entries) {
        // Skip virtual filesystems
        if (entry.fs_type == "proc" || entry.fs_type == "sysfs" || entry.fs_type == "devpts" ||
            entry.fs_type == "cgroup" || entry.fs_type == "cgroup2" || entry.fs_type == "securityfs" ||
            entry.fs_type == "debugfs" || entry.fs_type == "configfs" || entry.fs_type == "fusectl" ||
            entry.fs_type == "mqueue" || entry.fs_type == "hugetlbfs" || entry.fs_type == "pstore" ||
            entry.fs_type == "bpf" || entry.fs_type == "tracefs") {
            continue;
        }

        auto info = get_volume_info_impl(entry.mount_point);
        if (info) {
            volumes.push_back(std::move(*info));
        }
    }

    return volumes;
}

result<storage_location_type> get_storage_location_type_impl(const path &p) noexcept {
    auto mount = find_mount_for_path(p);
    if (!mount) {
        return std::unexpected(make_error(std::errc::no_such_file_or_directory));
    }

    const std::string &fs_type = mount->fs_type;
    const std::string &device = mount->device;

    if (fs_type == "nfs" || fs_type == "nfs4") {
        return storage_location_type::nfs;
    }
    if (fs_type == "cifs" || fs_type == "smbfs") {
        return storage_location_type::network_share;
    }
    if (fs_type == "tmpfs" || fs_type == "ramfs") {
        return storage_location_type::ram;
    }
    if (fs_type.find("fuse") != std::string::npos) {
        return storage_location_type::fuse;
    }
    if (device.find("/dev/loop") == 0) {
        return storage_location_type::loop;
    }
    if (device.find("/dev/sr") == 0 || device.find("/dev/cdrom") == 0) {
        return storage_location_type::optical;
    }

    // Check if removable
    if (device.find("/dev/sd") == 0) {
        std::string dev_name = device.substr(5);
        if (!dev_name.empty() && std::isdigit(dev_name.back())) {
            dev_name.pop_back();
        }
        std::string removable_path = "/sys/block/" + dev_name + "/removable";
        std::ifstream removable_file(removable_path);
        if (removable_file) {
            int removable = 0;
            removable_file >> removable;
            if (removable == 1) {
                return storage_location_type::removable;
            }
        }
    }

    return storage_location_type::local;
}

result<bool> is_network_path_impl(const path &p) noexcept {
    auto mount = find_mount_for_path(p);
    if (!mount) {
        return std::unexpected(make_error(std::errc::no_such_file_or_directory));
    }

    const std::string &fs_type = mount->fs_type;
    return fs_type == "nfs" || fs_type == "nfs4" || fs_type == "cifs" || fs_type == "smbfs";
}

result<bool> is_unc_path_impl(const path &p) noexcept {
    // UNC paths are Windows-specific
    return false;
}

result<path> get_unc_path_impl(const path &p) noexcept {
    return std::unexpected(make_error(std::errc::not_supported));
}

result<network_share_info> get_network_share_info_impl(const path &p) noexcept {
    network_share_info info;

    auto mount = find_mount_for_path(p);
    if (!mount) {
        return std::unexpected(make_error(std::errc::no_such_file_or_directory));
    }

    const std::string &fs_type = mount->fs_type;
    const std::string &device = mount->device;

    if (fs_type == "nfs" || fs_type == "nfs4") {
        info.protocol = (fs_type == "nfs4") ? network_protocol::nfs4 : network_protocol::nfs;

        // Parse NFS path: server:/path
        std::size_t colon = device.find(':');
        if (colon != std::string::npos) {
            info.server = device.substr(0, colon);
            info.remote_path = device.substr(colon + 1);
        }
    } else if (fs_type == "cifs" || fs_type == "smbfs") {
        info.protocol = network_protocol::smb;

        // Parse CIFS path: //server/share
        if (device.size() > 2 && device[0] == '/' && device[1] == '/') {
            std::size_t server_end = device.find('/', 2);
            if (server_end != std::string::npos) {
                info.server = device.substr(2, server_end - 2);
                std::size_t share_end = device.find('/', server_end + 1);
                if (share_end == std::string::npos) {
                    info.share_name = device.substr(server_end + 1);
                } else {
                    info.share_name = device.substr(server_end + 1, share_end - server_end - 1);
                    info.remote_path = device.substr(share_end);
                }
            }
        }
    } else {
        return std::unexpected(make_error(std::errc::not_supported));
    }

    info.local_mount_point = mount->mount_point;
    info.is_connected = true;

    return info;
}

result<std::vector<network_share_info>> list_network_shares_impl() noexcept {
    std::vector<network_share_info> shares;

    auto entries = get_mount_entries();
    for (const auto &entry : entries) {
        if (entry.fs_type == "nfs" || entry.fs_type == "nfs4" || entry.fs_type == "cifs" || entry.fs_type == "smbfs") {
            auto info = get_network_share_info_impl(entry.mount_point);
            if (info) {
                shares.push_back(std::move(*info));
            }
        }
    }

    return shares;
}

// ============================================================================
// 6.9 可移动存储设备
// ============================================================================

result<std::vector<removable_device_info>> list_removable_devices_impl() noexcept {
    std::vector<removable_device_info> devices;

    // Check /sys/block for removable devices
    const std::string sys_block = "/sys/block";
    try {
        for (const auto &entry : std::filesystem::directory_iterator(sys_block)) {
            std::string device_name = entry.path().filename().string();

            // Check if removable
            std::string removable_path = sys_block + "/" + device_name + "/removable";
            std::ifstream removable_file(removable_path);
            int is_removable = 0;
            if (removable_file >> is_removable && is_removable == 1) {
                removable_device_info info;
                info.device_path = "/dev/" + device_name;
                info.device_type = removable_device_type::usb_drive;

                // Get size
                std::string size_path = sys_block + "/" + device_name + "/size";
                std::ifstream size_file(size_path);
                std::uintmax_t sectors = 0;
                if (size_file >> sectors) {
                    info.size = sectors * 512;
                }

                // Check if mounted
                auto entries = get_mount_entries();
                for (const auto &mount : entries) {
                    if (mount.device.find(device_name) != std::string::npos) {
                        info.mount_point = mount.mount_point;
                        info.is_mounted = true;
                        info.fs_type = parse_filesystem_name(mount.fs_type);
                        break;
                    }
                }

                info.is_ejectable = true;
                info.bus_type = "USB";

                devices.push_back(info);
            }
        }
    } catch (...) {
        // Ignore errors
    }

    return devices;
}

result<removable_device_info> get_removable_device_info_impl(const path &p) noexcept {
    auto devices = list_removable_devices_impl();
    if (!devices) {
        return std::unexpected(devices.error());
    }

    std::string path_str = p.string();
    for (const auto &dev : *devices) {
        if (dev.is_mounted && path_str.find(dev.mount_point.string()) == 0) {
            return dev;
        }
    }

    return std::unexpected(make_error(std::errc::no_such_device));
}

result<bool> is_device_ready_impl(const path &p) noexcept {
    std::error_code ec;
    return std::filesystem::exists(p, ec) && !ec;
}

result<bool> is_media_present_impl(const path &p) noexcept {
    return is_device_ready_impl(p);
}

// ============================================================================
// 6.10 云存储挂载
// ============================================================================

result<bool> is_cloud_path_impl(const path &p) noexcept {
    std::string path_str = p.string();

    if (path_str.find("Dropbox") != std::string::npos) {
        return true;
    }
    if (path_str.find("Google Drive") != std::string::npos) {
        return true;
    }
    if (path_str.find("OneDrive") != std::string::npos) {
        return true;
    }
    if (path_str.find("Nextcloud") != std::string::npos) {
        return true;
    }
    if (path_str.find("ownCloud") != std::string::npos) {
        return true;
    }

    // Check for rclone mounts
    auto entries = get_mount_entries();
    for (const auto &entry : entries) {
        if (entry.fs_type == "fuse.rclone" && path_str.find(entry.mount_point) == 0) {
            return true;
        }
    }

    return false;
}

result<cloud_provider> get_cloud_provider_impl(const path &p) noexcept {
    std::string path_str = p.string();

    if (path_str.find("Dropbox") != std::string::npos) {
        return cloud_provider::dropbox;
    }
    if (path_str.find("Google Drive") != std::string::npos) {
        return cloud_provider::google_drive;
    }
    if (path_str.find("OneDrive") != std::string::npos) {
        return cloud_provider::onedrive;
    }
    if (path_str.find("Nextcloud") != std::string::npos) {
        return cloud_provider::nextcloud;
    }
    if (path_str.find("ownCloud") != std::string::npos) {
        return cloud_provider::owncloud;
    }

    return cloud_provider::unknown;
}

result<cloud_storage_info> get_cloud_storage_info_impl(const path &p) noexcept {
    cloud_storage_info info;
    auto provider = get_cloud_provider_impl(p);
    if (provider) {
        info.provider = *provider;
    }

    if (info.provider == cloud_provider::unknown) {
        return std::unexpected(make_error(std::errc::no_such_file_or_directory));
    }

    info.mount_point = p;
    info.is_online = true;

    return info;
}

result<std::vector<cloud_storage_info>> list_cloud_mounts_impl() noexcept {
    std::vector<cloud_storage_info> mounts;

    // Check common cloud storage paths in home directory
    const char *home = getenv("HOME");
    if (home) {
        path home_path(home);

        path dropbox = home_path / "Dropbox";
        if (std::filesystem::exists(dropbox)) {
            cloud_storage_info info;
            info.provider = cloud_provider::dropbox;
            info.mount_point = dropbox;
            info.is_online = true;
            mounts.push_back(info);
        }

        path nextcloud = home_path / "Nextcloud";
        if (std::filesystem::exists(nextcloud)) {
            cloud_storage_info info;
            info.provider = cloud_provider::nextcloud;
            info.mount_point = nextcloud;
            info.is_online = true;
            mounts.push_back(info);
        }
    }

    return mounts;
}

// ============================================================================
// 6.11 虚拟文件系统
// ============================================================================

result<bool> is_virtual_filesystem_impl(const path &p) noexcept {
    auto mount = find_mount_entry(p.string());
    if (!mount) {
        return false;
    }

    const std::string &fs = mount->fs_type;
    if (fs.find("fuse") == 0) {
        return true;
    }
    if (fs == "tmpfs" || fs == "ramfs") {
        return true;
    }
    if (fs == "overlay" || fs == "aufs" || fs == "unionfs") {
        return true;
    }
    if (fs == "proc" || fs == "sysfs" || fs == "devtmpfs") {
        return true;
    }

    return false;
}

result<bool> is_fuse_mount_impl(const path &p) noexcept {
    auto mount = find_mount_entry(p.string());
    if (!mount) {
        return false;
    }

    return mount->fs_type.find("fuse") == 0;
}

result<bool> is_encrypted_mount_impl(const path &p) noexcept {
    auto mount = find_mount_entry(p.string());
    if (!mount) {
        return false;
    }

    const std::string &fs = mount->fs_type;
    if (fs == "fuse.encfs" || fs == "fuse.gocryptfs" || fs == "fuse.veracrypt") {
        return true;
    }

    // Check for LUKS
    if (mount->device.find("/dev/mapper/") == 0) {
        return true;
    }

    return false;
}

result<virtual_fs_info> get_virtual_fs_info_impl(const path &p) noexcept {
    virtual_fs_info info;

    auto mount = find_mount_entry(p.string());
    if (!mount) {
        return std::unexpected(make_error(std::errc::no_such_file_or_directory));
    }

    info.mount_point = mount->mount_point;
    info.source = mount->device;

    const std::string &fs = mount->fs_type;
    if (fs.find("fuse") == 0) {
        info.type = virtual_fs_type::fuse;
    } else if (fs == "tmpfs") {
        info.type = virtual_fs_type::tmpfs;
    } else if (fs == "ramfs") {
        info.type = virtual_fs_type::ramfs;
    } else if (fs == "overlay") {
        info.type = virtual_fs_type::overlay;
    } else if (fs == "proc") {
        info.type = virtual_fs_type::procfs;
    } else if (fs == "sysfs") {
        info.type = virtual_fs_type::sysfs;
    }

    if (fs == "fuse.sshfs") {
        info.type = virtual_fs_type::sshfs;
    } else if (fs == "fuse.encfs") {
        info.type = virtual_fs_type::encfs;
        info.is_encrypted = true;
    } else if (fs == "fuse.gocryptfs") {
        info.type = virtual_fs_type::fuse;
        info.is_encrypted = true;
    }

    return info;
}

result<std::vector<virtual_fs_info>> list_virtual_mounts_impl() noexcept {
    std::vector<virtual_fs_info> mounts;

    auto entries = get_mount_entries();
    for (const auto &entry : entries) {
        const std::string &fs = entry.fs_type;
        if (fs.find("fuse") == 0 || fs == "tmpfs" || fs == "ramfs" || fs == "overlay" || fs == "proc" ||
            fs == "sysfs" || fs == "devtmpfs") {
            auto info = get_virtual_fs_info_impl(entry.mount_point);
            if (info) {
                mounts.push_back(std::move(*info));
            }
        }
    }

    return mounts;
}

// ============================================================================
// 6.12 Mount Point Management
// ============================================================================

namespace {
storage_location_type get_storage_type_from_fs(const std::string &fs_type, const std::string &device) {
    if (fs_type == "nfs" || fs_type == "nfs4" || fs_type == "cifs" || fs_type == "smbfs") {
        return storage_location_type::network_share;
    }
    if (fs_type == "tmpfs" || fs_type == "ramfs") {
        return storage_location_type::ram;
    }
    if (fs_type.find("fuse") == 0 || fs_type == "overlay" || fs_type == "proc" || fs_type == "sysfs") {
        return storage_location_type::virtual_fs;
    }
    if (device.find("/dev/loop") == 0) {
        return storage_location_type::loop;
    }
    if (device.find("/dev/sd") == 0 || device.find("/dev/nvme") == 0 || device.find("/dev/hd") == 0) {
        // Check if removable via sysfs
        std::string dev_name = device.substr(5); // Remove "/dev/"
        // Remove partition number
        while (!dev_name.empty() && std::isdigit(dev_name.back())) {
            dev_name.pop_back();
        }
        if (dev_name.ends_with("p")) {
            dev_name.pop_back(); // nvme0n1p1 -> nvme0n1
        }

        std::string removable_path = "/sys/block/" + dev_name + "/removable";
        std::ifstream f(removable_path);
        std::string val;
        if (f >> val && val == "1") {
            return storage_location_type::removable;
        }
        return storage_location_type::local;
    }
    return storage_location_type::unknown;
}

filesystem_type get_fs_type_from_name_linux(const std::string &fs_name) {
    if (fs_name == "ext4") {
        return filesystem_type::ext4;
    }
    if (fs_name == "ext3") {
        return filesystem_type::ext3;
    }
    if (fs_name == "ext2") {
        return filesystem_type::ext2;
    }
    if (fs_name == "xfs") {
        return filesystem_type::xfs;
    }
    if (fs_name == "btrfs") {
        return filesystem_type::btrfs;
    }
    if (fs_name == "zfs") {
        return filesystem_type::zfs;
    }
    if (fs_name == "ntfs" || fs_name == "ntfs3" || fs_name == "fuseblk") {
        return filesystem_type::ntfs;
    }
    if (fs_name == "vfat" || fs_name == "fat32") {
        return filesystem_type::fat32;
    }
    if (fs_name == "exfat") {
        return filesystem_type::exfat;
    }
    if (fs_name == "nfs" || fs_name == "nfs4") {
        return filesystem_type::nfs;
    }
    if (fs_name == "cifs" || fs_name == "smbfs") {
        return filesystem_type::cifs;
    }
    if (fs_name == "tmpfs") {
        return filesystem_type::tmpfs;
    }
    if (fs_name == "overlay") {
        return filesystem_type::overlay;
    }
    return filesystem_type::unknown;
}

void parse_mount_options(const std::string &opts_str, mount_point_info &info) {
    std::istringstream iss(opts_str);
    std::string opt;
    while (std::getline(iss, opt, ',')) {
        info.options.push_back(opt);
        if (opt == "ro") {
            info.is_readonly = true;
        } else if (opt == "noexec") {
            info.is_noexec = true;
        } else if (opt == "nosuid") {
            info.is_nosuid = true;
        } else if (opt == "nodev") {
            info.is_nodev = true;
        } else if (opt == "bind") {
            info.is_bind = true;
        } else if (opt == "loop") {
            info.is_loop = true;
        }
    }
}
} // namespace

result<std::vector<mount_point_info>> list_all_mounts_impl() noexcept {
    std::vector<mount_point_info> mounts;

    auto entries = get_mount_entries();
    for (const auto &entry : entries) {
        mount_point_info info;
        info.mount_path = entry.mount_point;
        info.device = entry.device;
        info.fs_type = get_fs_type_from_name_linux(entry.fs_type);
        info.storage_type = get_storage_type_from_fs(entry.fs_type, entry.device);
        parse_mount_options(entry.options, info);

        // Determine parent mount
        for (const auto &other : entries) {
            if (other.mount_point == entry.mount_point) {
                continue;
            }
            std::string other_mp = other.mount_point;
            std::string this_mp = entry.mount_point;
            if (this_mp.starts_with(other_mp) && (other_mp == "/" || this_mp[other_mp.length()] == '/')) {
                if (info.parent_mount.empty() || other_mp.length() > info.parent_mount.string().length()) {
                    info.parent_mount = other_mp;
                }
            }
        }

        mounts.push_back(info);
    }

    return mounts;
}

result<mount_point_info> get_mount_point_info_impl(const path &p) noexcept {
    std::error_code ec;
    path abs_path = std::filesystem::absolute(p, ec);
    if (ec) {
        return std::unexpected(make_error(ec.value(), ec.category()));
    }

    auto all_mounts = list_all_mounts_impl();
    if (!all_mounts) {
        return std::unexpected(all_mounts.error());
    }

    mount_point_info best_match;
    size_t best_len = 0;

    for (const auto &m : *all_mounts) {
        std::string mount_str = m.mount_path.string();
        std::string path_str = abs_path.string();

        if (path_str.starts_with(mount_str)) {
            if (mount_str == "/" || path_str.length() == mount_str.length() || path_str[mount_str.length()] == '/') {
                if (mount_str.length() > best_len) {
                    best_len = mount_str.length();
                    best_match = m;
                }
            }
        }
    }

    if (best_len == 0) {
        return std::unexpected(make_error(std::errc::no_such_file_or_directory));
    }
    return best_match;
}

result<bool> is_mount_point_impl(const path &p) noexcept {
    std::error_code ec;
    path abs_path = std::filesystem::absolute(p, ec);
    if (ec) {
        return std::unexpected(make_error(ec.value(), ec.category()));
    }

    auto all_mounts = list_all_mounts_impl();
    if (!all_mounts) {
        return std::unexpected(all_mounts.error());
    }

    std::string path_str = abs_path.string();
    for (const auto &m : *all_mounts) {
        if (m.mount_path.string() == path_str) {
            return true;
        }
    }
    return false;
}

} // namespace frappe::detail

#endif // FRAPPE_PLATFORM_LINUX
