#ifdef FRAPPE_PLATFORM_MACOS

#include "frappe/filesystem.hpp"

#include <cstring>
#include <fstream>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/statvfs.h>

namespace frappe::detail {

namespace {
filesystem_type parse_filesystem_name(const std::string &name) {
    if (name == "apfs") {
        return filesystem_type::apfs;
    }
    if (name == "hfs" || name == "hfs+") {
        return filesystem_type::hfs_plus;
    }
    if (name == "msdos" || name == "fat32") {
        return filesystem_type::fat32;
    }
    if (name == "exfat") {
        return filesystem_type::exfat;
    }
    if (name == "ntfs") {
        return filesystem_type::ntfs;
    }
    if (name == "nfs") {
        return filesystem_type::nfs;
    }
    if (name == "smbfs" || name == "cifs") {
        return filesystem_type::smb;
    }
    if (name == "webdav") {
        return filesystem_type::webdav;
    }
    if (name == "devfs") {
        return filesystem_type::devfs;
    }
    if (name.find("fuse") != std::string::npos) {
        return filesystem_type::fuse;
    }

    return filesystem_type::unknown;
}

struct mount_entry {
    std::string device;
    std::string mount_point;
    std::string fs_type;
    uint32_t flags;
};

std::vector<mount_entry> get_mount_entries() {
    std::vector<mount_entry> entries;

    struct statfs *mounts = nullptr;
    int count = getmntinfo(&mounts, MNT_NOWAIT);

    if (count > 0 && mounts != nullptr) {
        for (int i = 0; i < count; ++i) {
            mount_entry entry;
            entry.device = mounts[i].f_mntfromname;
            entry.mount_point = mounts[i].f_mntonname;
            entry.fs_type = mounts[i].f_fstypename;
            entry.flags = mounts[i].f_flags;
            entries.push_back(entry);
        }
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
    case filesystem_type::apfs:
        features.case_sensitive = false; // Default, can be case-sensitive
        features.case_preserving = true;
        features.supports_symlinks = true;
        features.supports_hard_links = true;
        features.supports_permissions = true;
        features.supports_acl = true;
        features.supports_extended_attributes = true;
        features.supports_sparse_files = true;
        features.supports_compression = true;
        features.supports_encryption = true;
        features.max_path_length = 1024;
        break;

    case filesystem_type::hfs_plus:
        features.case_sensitive = false;
        features.case_preserving = true;
        features.supports_symlinks = true;
        features.supports_hard_links = true;
        features.supports_permissions = true;
        features.supports_acl = true;
        features.supports_extended_attributes = true;
        features.supports_compression = true;
        features.max_path_length = 1024;
        break;

    case filesystem_type::fat32:
    case filesystem_type::exfat:
        features.case_sensitive = false;
        features.case_preserving = true;
        features.supports_symlinks = false;
        features.supports_hard_links = false;
        features.supports_permissions = false;
        features.max_filename_length = 255;
        features.max_path_length = 260;
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

    return info;
}

result<std::vector<volume_info>> list_volumes_impl() noexcept {
    std::vector<volume_info> volumes;

    auto entries = get_mount_entries();
    for (const auto &entry : entries) {
        // Skip virtual filesystems
        if (entry.fs_type == "devfs" || entry.fs_type == "autofs" || entry.mount_point.find("/dev") == 0) {
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

    if (fs_type == "nfs") {
        return storage_location_type::nfs;
    }
    if (fs_type == "smbfs" || fs_type == "cifs") {
        return storage_location_type::network_share;
    }
    if (fs_type == "webdav") {
        return storage_location_type::network_share;
    }
    if (fs_type.find("fuse") != std::string::npos) {
        return storage_location_type::fuse;
    }
    if (device.find("/dev/disk") == 0) {
        // Check if external/removable
        // On macOS, external drives typically have higher disk numbers
        return storage_location_type::local;
    }

    return storage_location_type::local;
}

result<bool> is_network_path_impl(const path &p) noexcept {
    auto mount = find_mount_for_path(p);
    if (!mount) {
        return std::unexpected(make_error(std::errc::no_such_file_or_directory));
    }

    const std::string &fs_type = mount->fs_type;
    return fs_type == "nfs" || fs_type == "smbfs" || fs_type == "cifs" || fs_type == "webdav" || fs_type == "afpfs";
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

    if (fs_type == "nfs") {
        info.protocol = network_protocol::nfs;

        // Parse NFS path: server:/path
        std::size_t colon = device.find(':');
        if (colon != std::string::npos) {
            info.server = device.substr(0, colon);
            info.remote_path = device.substr(colon + 1);
        }
    } else if (fs_type == "smbfs" || fs_type == "cifs") {
        info.protocol = network_protocol::smb;

        // Parse SMB path: //user@server/share
        if (device.size() > 2 && device[0] == '/' && device[1] == '/') {
            std::string rest = device.substr(2);

            // Check for user@
            std::size_t at_pos = rest.find('@');
            if (at_pos != std::string::npos) {
                info.username = rest.substr(0, at_pos);
                rest = rest.substr(at_pos + 1);
            }

            std::size_t slash = rest.find('/');
            if (slash != std::string::npos) {
                info.server = rest.substr(0, slash);
                info.share_name = rest.substr(slash + 1);
            } else {
                info.server = rest;
            }
        }
    } else if (fs_type == "afpfs") {
        info.protocol = network_protocol::afp;
        // AFP parsing similar to SMB
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
        if (entry.fs_type == "nfs" || entry.fs_type == "smbfs" || entry.fs_type == "cifs" || entry.fs_type == "afpfs" ||
            entry.fs_type == "webdav") {
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

    // Check /Volumes for removable devices
    const path volumes_path("/Volumes");
    try {
        for (const auto &entry : std::filesystem::directory_iterator(volumes_path)) {
            if (!entry.is_directory()) {
                continue;
            }

            removable_device_info info;
            info.mount_point = entry.path();
            info.device_path = entry.path();
            info.is_mounted = true;
            info.device_type = removable_device_type::usb_drive;
            info.label = entry.path().filename().string();

            struct statfs st;
            if (statfs(entry.path().c_str(), &st) == 0) {
                info.size = static_cast<std::uintmax_t>(st.f_blocks) * st.f_bsize;
                info.fs_type = parse_filesystem_name(st.f_fstypename);

                // Check if removable (external volume)
                if (std::string(st.f_mntfromname).find("/dev/disk") == 0) {
                    info.is_ejectable = true;
                    devices.push_back(info);
                }
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
        if (path_str.find(dev.mount_point.string()) == 0) {
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
    if (path_str.find("iCloud") != std::string::npos) {
        return true;
    }
    if (path_str.find("Mobile Documents") != std::string::npos) {
        return true;
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
    if (path_str.find("iCloud") != std::string::npos || path_str.find("Mobile Documents") != std::string::npos) {
        return cloud_provider::icloud;
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

    const char *home = getenv("HOME");
    if (home) {
        path home_path(home);

        // iCloud
        path icloud = home_path / "Library/Mobile Documents/com~apple~CloudDocs";
        if (std::filesystem::exists(icloud)) {
            cloud_storage_info info;
            info.provider = cloud_provider::icloud;
            info.mount_point = icloud;
            info.is_online = true;
            mounts.push_back(info);
        }

        // Dropbox
        path dropbox = home_path / "Dropbox";
        if (std::filesystem::exists(dropbox)) {
            cloud_storage_info info;
            info.provider = cloud_provider::dropbox;
            info.mount_point = dropbox;
            info.is_online = true;
            mounts.push_back(info);
        }

        // Google Drive
        path gdrive = home_path / "Google Drive";
        if (std::filesystem::exists(gdrive)) {
            cloud_storage_info info;
            info.provider = cloud_provider::google_drive;
            info.mount_point = gdrive;
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
    auto mount = find_mount_for_path(p);
    if (!mount) {
        return false;
    }

    const std::string &fs = mount->fs_type;
    if (fs.find("fuse") == 0 || fs == "osxfuse" || fs == "macfuse") {
        return true;
    }
    if (fs == "devfs" || fs == "autofs") {
        return true;
    }

    return false;
}

result<bool> is_fuse_mount_impl(const path &p) noexcept {
    auto mount = find_mount_for_path(p);
    if (!mount) {
        return false;
    }

    const std::string &fs = mount->fs_type;
    return fs.find("fuse") == 0 || fs == "osxfuse" || fs == "macfuse";
}

result<bool> is_encrypted_mount_impl(const path &p) noexcept {
    auto mount = find_mount_for_path(p);
    if (!mount) {
        return false;
    }

    // Check for encrypted APFS volumes
    const std::string &fs = mount->fs_type;
    if (fs == "apfs") {
        // APFS encryption detection would require diskutil
        return false;
    }

    return false;
}

result<virtual_fs_info> get_virtual_fs_info_impl(const path &p) noexcept {
    virtual_fs_info info;

    auto mount = find_mount_for_path(p);
    if (!mount) {
        return std::unexpected(make_error(std::errc::no_such_file_or_directory));
    }

    info.mount_point = mount->mount_point;
    info.source = mount->device;

    const std::string &fs = mount->fs_type;
    if (fs.find("fuse") == 0 || fs == "osxfuse" || fs == "macfuse") {
        info.type = virtual_fs_type::fuse;
    }

    return info;
}

result<std::vector<virtual_fs_info>> list_virtual_mounts_impl() noexcept {
    std::vector<virtual_fs_info> mounts;

    auto entries = get_mount_entries();
    for (const auto &entry : entries) {
        const std::string &fs = entry.fs_type;
        if (fs.find("fuse") == 0 || fs == "osxfuse" || fs == "macfuse" || fs == "devfs" || fs == "autofs") {
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
storage_location_type get_storage_type_macos(const std::string &fs_type, const std::string &device) {
    if (fs_type == "nfs" || fs_type == "smbfs" || fs_type == "afpfs" || fs_type == "webdav") {
        return storage_location_type::network_share;
    }
    if (fs_type.find("fuse") == 0 || fs_type == "osxfuse" || fs_type == "macfuse" || fs_type == "devfs" ||
        fs_type == "autofs") {
        return storage_location_type::virtual_fs;
    }
    if (device.find("/dev/disk") == 0) {
        // External/removable detection would require IOKit
        return storage_location_type::local;
    }
    return storage_location_type::unknown;
}

filesystem_type get_fs_type_macos(const std::string &fs_name) {
    if (fs_name == "apfs") {
        return filesystem_type::apfs;
    }
    if (fs_name == "hfs") {
        return filesystem_type::hfs_plus;
    }
    if (fs_name == "msdos") {
        return filesystem_type::fat32;
    }
    if (fs_name == "exfat") {
        return filesystem_type::exfat;
    }
    if (fs_name == "ntfs") {
        return filesystem_type::ntfs;
    }
    if (fs_name == "nfs") {
        return filesystem_type::nfs;
    }
    if (fs_name == "smbfs") {
        return filesystem_type::smb;
    }
    if (fs_name == "afpfs") {
        return filesystem_type::cifs;
    }
    return filesystem_type::unknown;
}
} // namespace

result<std::vector<mount_point_info>> list_all_mounts_impl() noexcept {
    std::vector<mount_point_info> mounts;

    auto entries = get_mount_entries();
    for (const auto &entry : entries) {
        mount_point_info info;
        info.mount_path = entry.mount_point;
        info.device = entry.device;
        info.fs_type = get_fs_type_macos(entry.fs_type);
        info.storage_type = get_storage_type_macos(entry.fs_type, entry.device);
        info.is_readonly = entry.is_readonly;

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

#endif // FRAPPE_PLATFORM_MACOS
