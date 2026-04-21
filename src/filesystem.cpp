#include "frappe/filesystem.hpp"

#include <algorithm>
#include <cctype>
#include <functional>

namespace frappe {

    namespace detail {
        result<filesystem_type> get_filesystem_type_impl(const path &p) noexcept;
        result<filesystem_features> get_filesystem_features_impl(const path &p) noexcept;
        result<volume_info> get_volume_info_impl(const path &p) noexcept;
        result<std::vector<volume_info>> list_volumes_impl() noexcept;
        result<storage_location_type> get_storage_location_type_impl(const path &p) noexcept;
        result<bool> is_network_path_impl(const path &p) noexcept;
        result<bool> is_unc_path_impl(const path &p) noexcept;
        result<path> get_unc_path_impl(const path &p) noexcept;
        result<network_share_info> get_network_share_info_impl(const path &p) noexcept;
        result<std::vector<network_share_info>> list_network_shares_impl() noexcept;
#ifdef _WIN32
        result<std::vector<network_share_info>> list_mapped_drives_impl() noexcept;
#endif
    } // namespace detail

    // ============================================================================
    // 6.1 磁盘空间
    // ============================================================================

    result<space_info> space(const path &p) noexcept {
        std::error_code ec;
        auto info = std::filesystem::space(p, ec);
        if (ec) {
            return std::unexpected(ec);
        }
        return info;
    }

    // ============================================================================
    // 6.2 文件系统类型
    // ============================================================================

    std::string_view filesystem_type_name(filesystem_type type) noexcept {
        switch (type) {
        case filesystem_type::unknown:
            return "unknown";
        case filesystem_type::ntfs:
            return "ntfs";
        case filesystem_type::fat:
            return "fat";
        case filesystem_type::fat32:
            return "fat32";
        case filesystem_type::exfat:
            return "exfat";
        case filesystem_type::refs:
            return "refs";
        case filesystem_type::ext2:
            return "ext2";
        case filesystem_type::ext3:
            return "ext3";
        case filesystem_type::ext4:
            return "ext4";
        case filesystem_type::xfs:
            return "xfs";
        case filesystem_type::btrfs:
            return "btrfs";
        case filesystem_type::zfs:
            return "zfs";
        case filesystem_type::apfs:
            return "apfs";
        case filesystem_type::hfs_plus:
            return "hfs+";
        case filesystem_type::nfs:
            return "nfs";
        case filesystem_type::smb:
            return "smb";
        case filesystem_type::cifs:
            return "cifs";
        case filesystem_type::webdav:
            return "webdav";
        case filesystem_type::tmpfs:
            return "tmpfs";
        case filesystem_type::ramfs:
            return "ramfs";
        case filesystem_type::devfs:
            return "devfs";
        case filesystem_type::procfs:
            return "procfs";
        case filesystem_type::sysfs:
            return "sysfs";
        case filesystem_type::fuse:
            return "fuse";
        case filesystem_type::overlay:
            return "overlay";
        default:
            return "unknown";
        }
    }

    result<filesystem_type> get_filesystem_type(const path &p) noexcept {
        return detail::get_filesystem_type_impl(p);
    }

    result<filesystem_features> get_filesystem_features(const path &p) noexcept {
        return detail::get_filesystem_features_impl(p);
    }

    // ============================================================================
    // 6.5 卷信息
    // ============================================================================

    result<volume_info> get_volume_info(const path &p) noexcept {
        return detail::get_volume_info_impl(p);
    }

    result<std::vector<volume_info>> list_volumes() noexcept {
        return detail::list_volumes_impl();
    }

    // ============================================================================
    // 6.6 存储位置类型
    // ============================================================================

    std::string_view storage_location_type_name(storage_location_type type) noexcept {
        switch (type) {
        case storage_location_type::unknown:
            return "unknown";
        case storage_location_type::local:
            return "local";
        case storage_location_type::removable:
            return "removable";
        case storage_location_type::optical:
            return "optical";
        case storage_location_type::network_share:
            return "network_share";
        case storage_location_type::network_drive:
            return "network_drive";
        case storage_location_type::nfs:
            return "nfs";
        case storage_location_type::cloud:
            return "cloud";
        case storage_location_type::virtual_fs:
            return "virtual";
        case storage_location_type::fuse:
            return "fuse";
        case storage_location_type::loop:
            return "loop";
        case storage_location_type::ram:
            return "ram";
        default:
            return "unknown";
        }
    }

    result<storage_location_type> get_storage_location_type(const path &p) noexcept {
        return detail::get_storage_location_type_impl(p);
    }

    result<bool> is_local_storage(const path &p) noexcept {
        auto type = get_storage_location_type(p);
        if (!type) {
            return std::unexpected(type.error());
        }
        return *type == storage_location_type::local;
    }

    result<bool> is_removable_storage(const path &p) noexcept {
        auto type = get_storage_location_type(p);
        if (!type) {
            return std::unexpected(type.error());
        }
        return *type == storage_location_type::removable || *type == storage_location_type::optical;
    }

    result<bool> is_network_storage(const path &p) noexcept {
        auto type = get_storage_location_type(p);
        if (!type) {
            return std::unexpected(type.error());
        }
        return *type == storage_location_type::network_share || *type == storage_location_type::network_drive ||
               *type == storage_location_type::nfs;
    }

    result<bool> is_virtual_storage(const path &p) noexcept {
        auto type = get_storage_location_type(p);
        if (!type) {
            return std::unexpected(type.error());
        }
        return *type == storage_location_type::virtual_fs || *type == storage_location_type::fuse ||
               *type == storage_location_type::ram || *type == storage_location_type::loop;
    }

    // ============================================================================
    // 6.8 网络文件系统
    // ============================================================================

    std::string_view network_protocol_name(network_protocol proto) noexcept {
        switch (proto) {
        case network_protocol::unknown:
            return "unknown";
        case network_protocol::smb:
            return "smb";
        case network_protocol::smb1:
            return "smb1";
        case network_protocol::smb2:
            return "smb2";
        case network_protocol::smb3:
            return "smb3";
        case network_protocol::nfs:
            return "nfs";
        case network_protocol::nfs3:
            return "nfs3";
        case network_protocol::nfs4:
            return "nfs4";
        case network_protocol::webdav:
            return "webdav";
        case network_protocol::ftp:
            return "ftp";
        case network_protocol::sftp:
            return "sftp";
        case network_protocol::afp:
            return "afp";
        default:
            return "unknown";
        }
    }

    result<bool> is_network_path(const path &p) noexcept {
        return detail::is_network_path_impl(p);
    }

    result<bool> is_remote_filesystem(const path &p) noexcept {
        auto fs_type = get_filesystem_type(p);
        if (!fs_type) {
            return std::unexpected(fs_type.error());
        }
        return *fs_type == filesystem_type::nfs || *fs_type == filesystem_type::smb ||
               *fs_type == filesystem_type::cifs || *fs_type == filesystem_type::webdav;
    }

    result<bool> is_unc_path(const path &p) noexcept {
        return detail::is_unc_path_impl(p);
    }

    result<path> get_unc_path(const path &p) noexcept {
        return detail::get_unc_path_impl(p);
    }

    std::optional<unc_path_components> parse_unc_path(const path &p) noexcept {
        std::string path_str = p.string();

        // Check for UNC prefix
        if (path_str.size() < 3) {
            return std::nullopt;
        }
        if (!((path_str[0] == '\\' && path_str[1] == '\\') || (path_str[0] == '/' && path_str[1] == '/'))) {
            return std::nullopt;
        }

        // Find server name
        std::size_t server_start = 2;
        std::size_t server_end = path_str.find_first_of("\\/", server_start);
        if (server_end == std::string::npos) {
            return unc_path_components{path_str.substr(server_start), "", ""};
        }

        std::string server = path_str.substr(server_start, server_end - server_start);

        // Find share name
        std::size_t share_start = server_end + 1;
        if (share_start >= path_str.size()) {
            return unc_path_components{server, "", ""};
        }

        std::size_t share_end = path_str.find_first_of("\\/", share_start);
        if (share_end == std::string::npos) {
            return unc_path_components{server, path_str.substr(share_start), ""};
        }

        std::string share = path_str.substr(share_start, share_end - share_start);
        std::string remaining_path = path_str.substr(share_end);

        return unc_path_components{server, share, remaining_path};
    }

    result<network_share_info> get_network_share_info(const path &p) noexcept {
        return detail::get_network_share_info_impl(p);
    }

    result<std::vector<network_share_info>> list_network_shares() noexcept {
        return detail::list_network_shares_impl();
    }

    result<std::vector<network_share_info>> list_smb_shares() noexcept {
        auto all = list_network_shares();
        if (!all) {
            return std::unexpected(all.error());
        }

        std::vector<network_share_info> smb;
        for (const auto &s : *all) {
            if (s.protocol == network_protocol::smb || s.protocol == network_protocol::smb1 ||
                s.protocol == network_protocol::smb2 || s.protocol == network_protocol::smb3) {
                smb.push_back(s);
            }
        }
        return smb;
    }

    result<std::vector<network_share_info>> list_nfs_mounts() noexcept {
        auto all = list_network_shares();
        if (!all) {
            return std::unexpected(all.error());
        }

        std::vector<network_share_info> nfs;
        for (const auto &s : *all) {
            if (s.protocol == network_protocol::nfs || s.protocol == network_protocol::nfs3 ||
                s.protocol == network_protocol::nfs4) {
                nfs.push_back(s);
            }
        }
        return nfs;
    }

    result<bool> is_share_available(const path &p) noexcept {
        std::error_code ec;
        return std::filesystem::exists(p, ec) && !ec;
    }

    result<bool> is_share_connected(const path &p) noexcept {
        auto info = get_network_share_info(p);
        if (!info) {
            return std::unexpected(info.error());
        }
        return info->is_connected;
    }

    result<server_info> get_server_info(std::string_view server) noexcept {
        server_info info;
        info.hostname = std::string(server);

        // Basic reachability check - try to resolve the name
        std::error_code ec;
        path test_path;
#ifdef _WIN32
        test_path = "\\\\" + info.hostname;
#else
        test_path = "//" + info.hostname;
#endif
        info.is_reachable = std::filesystem::exists(test_path, ec) || !ec;

        return info;
    }

#ifdef _WIN32
    result<std::vector<network_share_info>> list_mapped_drives() noexcept {
        return detail::list_mapped_drives_impl();
    }
#endif

    // ============================================================================
    // 6.9 可移动存储设备
    // ============================================================================

    std::string_view removable_device_type_name(removable_device_type type) noexcept {
        switch (type) {
        case removable_device_type::unknown:
            return "unknown";
        case removable_device_type::usb_drive:
            return "usb_drive";
        case removable_device_type::usb_stick:
            return "usb_stick";
        case removable_device_type::sd_card:
            return "sd_card";
        case removable_device_type::cf_card:
            return "cf_card";
        case removable_device_type::external_hdd:
            return "external_hdd";
        case removable_device_type::external_ssd:
            return "external_ssd";
        case removable_device_type::optical:
            return "optical";
        case removable_device_type::floppy:
            return "floppy";
        case removable_device_type::mtp:
            return "mtp";
        case removable_device_type::ptp:
            return "ptp";
        default:
            return "unknown";
        }
    }

    namespace detail {
        result<std::vector<removable_device_info>> list_removable_devices_impl() noexcept;
        result<removable_device_info> get_removable_device_info_impl(const path &p) noexcept;
        result<bool> is_device_ready_impl(const path &p) noexcept;
        result<bool> is_media_present_impl(const path &p) noexcept;
    } // namespace detail

    result<std::vector<removable_device_info>> list_removable_devices() noexcept {
        return detail::list_removable_devices_impl();
    }

    result<std::vector<removable_device_info>> list_usb_devices() noexcept {
        auto all = list_removable_devices();
        if (!all) {
            return std::unexpected(all.error());
        }

        std::vector<removable_device_info> usb;
        for (const auto &d : *all) {
            if (d.device_type == removable_device_type::usb_drive ||
                d.device_type == removable_device_type::usb_stick ||
                d.device_type == removable_device_type::external_hdd ||
                d.device_type == removable_device_type::external_ssd) {
                usb.push_back(d);
            }
        }
        return usb;
    }

    result<std::vector<removable_device_info>> list_optical_drives() noexcept {
        auto all = list_removable_devices();
        if (!all) {
            return std::unexpected(all.error());
        }

        std::vector<removable_device_info> optical;
        for (const auto &d : *all) {
            if (d.device_type == removable_device_type::optical) {
                optical.push_back(d);
            }
        }
        return optical;
    }

    result<std::vector<removable_device_info>> list_card_readers() noexcept {
        auto all = list_removable_devices();
        if (!all) {
            return std::unexpected(all.error());
        }

        std::vector<removable_device_info> readers;
        for (const auto &d : *all) {
            if (d.device_type == removable_device_type::sd_card || d.device_type == removable_device_type::cf_card) {
                readers.push_back(d);
            }
        }
        return readers;
    }

    result<removable_device_info> get_removable_device_info(const path &p) noexcept {
        return detail::get_removable_device_info_impl(p);
    }

    result<bool> is_device_ready(const path &p) noexcept {
        return detail::is_device_ready_impl(p);
    }

    result<bool> is_media_present(const path &p) noexcept {
        return detail::is_media_present_impl(p);
    }

    std::string_view device_status_name(device_status status) noexcept {
        switch (status) {
        case device_status::unknown:
            return "unknown";
        case device_status::ready:
            return "ready";
        case device_status::not_ready:
            return "not_ready";
        case device_status::no_media:
            return "no_media";
        case device_status::error:
            return "error";
        case device_status::ejecting:
            return "ejecting";
        case device_status::busy:
            return "busy";
        default:
            return "unknown";
        }
    }

    result<device_status> get_device_status(const path &p) noexcept {
        auto ready = is_device_ready(p);
        if (!ready) {
            return std::unexpected(ready.error());
        }

        if (!*ready) {
            auto media = is_media_present(p);
            if (media && !*media) {
                return device_status::no_media;
            }
            return device_status::not_ready;
        }
        return device_status::ready;
    }

    // ============================================================================
    // 6.10 云存储挂载
    // ============================================================================

    std::string_view cloud_provider_name(cloud_provider provider) noexcept {
        switch (provider) {
        case cloud_provider::unknown:
            return "unknown";
        case cloud_provider::onedrive:
            return "onedrive";
        case cloud_provider::google_drive:
            return "google_drive";
        case cloud_provider::dropbox:
            return "dropbox";
        case cloud_provider::icloud:
            return "icloud";
        case cloud_provider::box:
            return "box";
        case cloud_provider::amazon_drive:
            return "amazon_drive";
        case cloud_provider::nextcloud:
            return "nextcloud";
        case cloud_provider::owncloud:
            return "owncloud";
        default:
            return "unknown";
        }
    }

    namespace detail {
        result<bool> is_cloud_path_impl(const path &p) noexcept;
        result<cloud_provider> get_cloud_provider_impl(const path &p) noexcept;
        result<cloud_storage_info> get_cloud_storage_info_impl(const path &p) noexcept;
        result<std::vector<cloud_storage_info>> list_cloud_mounts_impl() noexcept;
    } // namespace detail

    result<bool> is_cloud_path(const path &p) noexcept {
        return detail::is_cloud_path_impl(p);
    }

    result<cloud_provider> get_cloud_provider(const path &p) noexcept {
        return detail::get_cloud_provider_impl(p);
    }

    result<cloud_storage_info> get_cloud_storage_info(const path &p) noexcept {
        return detail::get_cloud_storage_info_impl(p);
    }

    result<std::vector<cloud_storage_info>> list_cloud_mounts() noexcept {
        return detail::list_cloud_mounts_impl();
    }

    // ============================================================================
    // 6.11 虚拟文件系统
    // ============================================================================

    std::string_view virtual_fs_type_name(virtual_fs_type type) noexcept {
        switch (type) {
        case virtual_fs_type::unknown:
            return "unknown";
        case virtual_fs_type::fuse:
            return "fuse";
        case virtual_fs_type::winfsp:
            return "winfsp";
        case virtual_fs_type::dokan:
            return "dokan";
        case virtual_fs_type::overlay:
            return "overlay";
        case virtual_fs_type::unionfs:
            return "unionfs";
        case virtual_fs_type::bindfs:
            return "bindfs";
        case virtual_fs_type::sshfs:
            return "sshfs";
        case virtual_fs_type::encfs:
            return "encfs";
        case virtual_fs_type::veracrypt:
            return "veracrypt";
        case virtual_fs_type::loop:
            return "loop";
        case virtual_fs_type::tmpfs:
            return "tmpfs";
        case virtual_fs_type::ramfs:
            return "ramfs";
        case virtual_fs_type::procfs:
            return "procfs";
        case virtual_fs_type::sysfs:
            return "sysfs";
        default:
            return "unknown";
        }
    }

    namespace detail {
        result<bool> is_virtual_filesystem_impl(const path &p) noexcept;
        result<bool> is_fuse_mount_impl(const path &p) noexcept;
        result<bool> is_encrypted_mount_impl(const path &p) noexcept;
        result<virtual_fs_info> get_virtual_fs_info_impl(const path &p) noexcept;
        result<std::vector<virtual_fs_info>> list_virtual_mounts_impl() noexcept;

        // 6.12 Mount point management
        result<std::vector<mount_point_info>> list_all_mounts_impl() noexcept;
        result<mount_point_info> get_mount_point_info_impl(const path &p) noexcept;
        result<bool> is_mount_point_impl(const path &p) noexcept;
    } // namespace detail

    result<bool> is_virtual_filesystem(const path &p) noexcept {
        return detail::is_virtual_filesystem_impl(p);
    }

    result<bool> is_fuse_mount(const path &p) noexcept {
        return detail::is_fuse_mount_impl(p);
    }

    result<bool> is_encrypted_mount(const path &p) noexcept {
        return detail::is_encrypted_mount_impl(p);
    }

    result<virtual_fs_info> get_virtual_fs_info(const path &p) noexcept {
        return detail::get_virtual_fs_info_impl(p);
    }

    result<std::vector<virtual_fs_info>> list_virtual_mounts() noexcept {
        return detail::list_virtual_mounts_impl();
    }

    // ============================================================================
    // 6.12 Mount Point Management
    // ============================================================================

    result<std::vector<mount_point_info>> list_all_mounts() noexcept {
        return detail::list_all_mounts_impl();
    }

    result<std::vector<mount_point_info>> list_local_mounts() noexcept {
        auto all_mounts = list_all_mounts();
        if (!all_mounts) {
            return std::unexpected(all_mounts.error());
        }

        std::vector<mount_point_info> local;
        for (const auto &m : *all_mounts) {
            if (m.storage_type == storage_location_type::local) {
                local.push_back(m);
            }
        }
        return local;
    }

    result<std::vector<mount_point_info>> list_network_mounts() noexcept {
        auto all_mounts = list_all_mounts();
        if (!all_mounts) {
            return std::unexpected(all_mounts.error());
        }

        std::vector<mount_point_info> network;
        for (const auto &m : *all_mounts) {
            if (m.storage_type == storage_location_type::network_share ||
                m.storage_type == storage_location_type::network_drive ||
                m.storage_type == storage_location_type::nfs) {
                network.push_back(m);
            }
        }
        return network;
    }

    result<std::vector<mount_point_info>> list_removable_mounts() noexcept {
        auto all_mounts = list_all_mounts();
        if (!all_mounts) {
            return std::unexpected(all_mounts.error());
        }

        std::vector<mount_point_info> removable;
        for (const auto &m : *all_mounts) {
            if (m.storage_type == storage_location_type::removable) {
                removable.push_back(m);
            }
        }
        return removable;
    }

    result<mount_point_info> get_mount_point_info(const path &p) noexcept {
        return detail::get_mount_point_info_impl(p);
    }

    result<path> find_mount_point(const path &p) noexcept {
        std::error_code ec;
        path abs_path = std::filesystem::absolute(p, ec);
        if (ec) {
            return std::unexpected(make_error(ec.value(), ec.category()));
        }

        auto all_mounts = list_all_mounts();
        if (!all_mounts) {
            return std::unexpected(all_mounts.error());
        }

        path best_match;
        size_t best_len = 0;

        for (const auto &m : *all_mounts) {
            std::string mount_str = m.mount_path.string();
            std::string path_str = abs_path.string();

            if (path_str.starts_with(mount_str)) {
                if (mount_str.length() > best_len) {
                    best_len = mount_str.length();
                    best_match = m.mount_path;
                }
            }
        }

        if (best_match.empty()) {
            return std::unexpected(make_error(std::errc::no_such_file_or_directory));
        }
        return best_match;
    }

    result<std::vector<path>> get_all_mount_points_under(const path &p) noexcept {
        std::error_code ec;
        path abs_path = std::filesystem::absolute(p, ec);
        if (ec) {
            return std::unexpected(make_error(ec.value(), ec.category()));
        }

        auto all_mounts = list_all_mounts();
        if (!all_mounts) {
            return std::unexpected(all_mounts.error());
        }

        std::vector<path> result;
        std::string base_str = abs_path.string();

        for (const auto &m : *all_mounts) {
            std::string mount_str = m.mount_path.string();
            if (mount_str.starts_with(base_str) && mount_str != base_str) {
                result.push_back(m.mount_path);
            }
        }
        return result;
    }

    // is_mount_point is defined in disk.cpp

    result<std::string> get_mount_source(const path &p) noexcept {
        auto info = get_mount_point_info(p);
        if (!info) {
            return std::unexpected(info.error());
        }
        return info->device;
    }

    result<mount_tree_node> get_mount_tree() noexcept {
        auto all_mounts = list_all_mounts();
        if (!all_mounts) {
            return std::unexpected(all_mounts.error());
        }

        // Find root mount
        mount_tree_node root;
        for (const auto &m : *all_mounts) {
            if (m.mount_path == "/" || m.mount_path == "C:\\" || m.mount_path == "C:/") {
                root.info = m;
                break;
            }
        }

        // Build tree recursively
        std::function<void(mount_tree_node &)> build_children = [&](mount_tree_node &node) {
            for (const auto &m : *all_mounts) {
                if (m.parent_mount == node.info.mount_path && m.mount_path != node.info.mount_path) {
                    mount_tree_node child;
                    child.info = m;
                    build_children(child);
                    node.children.push_back(std::move(child));
                }
            }
        };

        build_children(root);
        return root;
    }

    result<path> get_parent_mount(const path &p) noexcept {
        auto info = get_mount_point_info(p);
        if (!info) {
            return std::unexpected(info.error());
        }

        if (info->parent_mount.empty()) {
            return std::unexpected(make_error(std::errc::no_such_file_or_directory));
        }
        return info->parent_mount;
    }

    result<std::vector<path>> get_child_mounts(const path &p) noexcept {
        std::error_code ec;
        path abs_path = std::filesystem::absolute(p, ec);
        if (ec) {
            return std::unexpected(make_error(ec.value(), ec.category()));
        }

        auto all_mounts = list_all_mounts();
        if (!all_mounts) {
            return std::unexpected(all_mounts.error());
        }

        std::vector<path> children;
        for (const auto &m : *all_mounts) {
            if (m.parent_mount == abs_path) {
                children.push_back(m.mount_path);
            }
        }
        return children;
    }

    result<bool> is_submount(const path &p) noexcept {
        auto info = get_mount_point_info(p);
        if (!info) {
            return std::unexpected(info.error());
        }
        return !info->parent_mount.empty();
    }

} // namespace frappe
