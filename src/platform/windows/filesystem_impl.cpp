#ifdef FRAPPE_PLATFORM_WINDOWS

#    include "frappe/filesystem.hpp"

#    include <Windows.h>
#    include <algorithm>
#    include <cctype>
#    include <winnetwk.h>

#    pragma comment(lib, "mpr.lib")

namespace frappe::detail {

    namespace {
        filesystem_type parse_filesystem_name(const std::wstring &name) {
            std::string lower_name;
            lower_name.reserve(name.size());
            for (wchar_t c : name) {
                lower_name.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            }

            if (lower_name == "ntfs") {
                return filesystem_type::ntfs;
            }
            if (lower_name == "fat") {
                return filesystem_type::fat;
            }
            if (lower_name == "fat32") {
                return filesystem_type::fat32;
            }
            if (lower_name == "exfat") {
                return filesystem_type::exfat;
            }
            if (lower_name == "refs") {
                return filesystem_type::refs;
            }
            if (lower_name == "nfs") {
                return filesystem_type::nfs;
            }
            if (lower_name == "cifs" || lower_name == "smbfs") {
                return filesystem_type::smb;
            }

            return filesystem_type::unknown;
        }

        std::wstring get_volume_path(const path &p) {
            wchar_t volume_path[MAX_PATH] = {0};
            if (GetVolumePathNameW(p.c_str(), volume_path, MAX_PATH)) {
                return volume_path;
            }
            return L"";
        }
    } // namespace

    result<filesystem_type> get_filesystem_type_impl(const path &p) noexcept {
        auto volume_path = get_volume_path(p);
        if (volume_path.empty()) {
            return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
        }

        wchar_t fs_name[MAX_PATH] = {0};
        if (!GetVolumeInformationW(volume_path.c_str(), nullptr, 0, nullptr, nullptr, nullptr, fs_name, MAX_PATH)) {
            return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
        }

        return parse_filesystem_name(fs_name);
    }

    result<filesystem_features> get_filesystem_features_impl(const path &p) noexcept {
        filesystem_features features;

        auto volume_path = get_volume_path(p);
        if (volume_path.empty()) {
            return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
        }

        wchar_t fs_name[MAX_PATH] = {0};
        DWORD serial = 0;
        DWORD max_component = 0;
        DWORD flags = 0;

        if (!GetVolumeInformationW(volume_path.c_str(), nullptr, 0, &serial, &max_component, &flags, fs_name,
                                   MAX_PATH)) {
            return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
        }

        features.type = parse_filesystem_name(fs_name);
        features.max_filename_length = max_component;
        features.max_path_length = MAX_PATH;

        features.case_sensitive = (flags & FILE_CASE_SENSITIVE_SEARCH) != 0;
        features.case_preserving = (flags & FILE_CASE_PRESERVED_NAMES) != 0;
        features.supports_symlinks = (flags & FILE_SUPPORTS_REPARSE_POINTS) != 0;
        features.supports_hard_links = (flags & FILE_SUPPORTS_HARD_LINKS) != 0;
        features.supports_acl = (flags & FILE_PERSISTENT_ACLS) != 0;
        features.supports_permissions = features.supports_acl;
        features.supports_extended_attributes = (flags & FILE_NAMED_STREAMS) != 0;
        features.supports_sparse_files = (flags & FILE_SUPPORTS_SPARSE_FILES) != 0;
        features.supports_compression = (flags & FILE_FILE_COMPRESSION) != 0;
        features.supports_encryption = (flags & FILE_SUPPORTS_ENCRYPTION) != 0;
        features.supports_transactions = (flags & FILE_SUPPORTS_TRANSACTIONS) != 0;
        features.supports_quotas = (flags & FILE_VOLUME_QUOTAS) != 0;

        // Get cluster size
        DWORD sectors_per_cluster = 0;
        DWORD bytes_per_sector = 0;
        DWORD free_clusters = 0;
        DWORD total_clusters = 0;
        if (GetDiskFreeSpaceW(volume_path.c_str(), &sectors_per_cluster, &bytes_per_sector, &free_clusters,
                              &total_clusters)) {
            features.block_size = bytes_per_sector;
            features.cluster_size = sectors_per_cluster * bytes_per_sector;
        }

        return features;
    }

    result<volume_info> get_volume_info_impl(const path &p) noexcept {
        volume_info info;

        auto volume_path = get_volume_path(p);
        if (volume_path.empty()) {
            return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
        }

        info.mount_point = volume_path;

        wchar_t volume_name[MAX_PATH] = {0};
        wchar_t fs_name[MAX_PATH] = {0};
        DWORD serial = 0;
        DWORD max_component = 0;
        DWORD flags = 0;

        if (GetVolumeInformationW(volume_path.c_str(), volume_name, MAX_PATH, &serial, &max_component, &flags, fs_name,
                                  MAX_PATH)) {
            std::wstring wname(volume_name);
            info.label = std::string(wname.begin(), wname.end());
            info.fs_type = parse_filesystem_name(fs_name);

            // Format serial number
            char serial_str[32];
            snprintf(serial_str, sizeof(serial_str), "%04lX-%04lX", (serial >> 16) & 0xFFFF, serial & 0xFFFF);
            info.serial_number = serial_str;
        }

        // Get volume GUID path
        wchar_t guid_path[MAX_PATH] = {0};
        if (GetVolumeNameForVolumeMountPointW(volume_path.c_str(), guid_path, MAX_PATH)) {
            std::wstring wguid(guid_path);
            info.name = std::string(wguid.begin(), wguid.end());

            // Extract UUID from GUID path
            if (wguid.size() > 10) {
                std::size_t start = wguid.find(L'{');
                std::size_t end = wguid.find(L'}');
                if (start != std::wstring::npos && end != std::wstring::npos && end > start) {
                    std::wstring uuid_w = wguid.substr(start + 1, end - start - 1);
                    info.uuid = std::string(uuid_w.begin(), uuid_w.end());
                }
            }
        }

        // Get space info
        ULARGE_INTEGER free_bytes, total_bytes, available_bytes;
        if (GetDiskFreeSpaceExW(volume_path.c_str(), &available_bytes, &total_bytes, &free_bytes)) {
            info.total_size = total_bytes.QuadPart;
            info.free_size = free_bytes.QuadPart;
            info.available_size = available_bytes.QuadPart;
        }

        return info;
    }

    result<std::vector<volume_info>> list_volumes_impl() noexcept {
        std::vector<volume_info> volumes;

        DWORD drives = GetLogicalDrives();
        if (drives == 0) {
            return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
        }

        for (int i = 0; i < 26; ++i) {
            if (drives & (1 << i)) {
                wchar_t drive_path[4] = {static_cast<wchar_t>(L'A' + i), L':', L'\\', L'\0'};

                UINT drive_type = GetDriveTypeW(drive_path);
                if (drive_type == DRIVE_NO_ROOT_DIR) {
                    continue;
                }

                auto info = get_volume_info_impl(drive_path);
                if (info) {
                    info->device_path = std::string(1, 'A' + i) + ":";
                    volumes.push_back(std::move(*info));
                }
            }
        }

        return volumes;
    }

    result<storage_location_type> get_storage_location_type_impl(const path &p) noexcept {
        auto volume_path = get_volume_path(p);
        if (volume_path.empty()) {
            return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
        }

        UINT drive_type = GetDriveTypeW(volume_path.c_str());

        switch (drive_type) {
        case DRIVE_FIXED:
            return storage_location_type::local;
        case DRIVE_REMOVABLE:
            return storage_location_type::removable;
        case DRIVE_CDROM:
            return storage_location_type::optical;
        case DRIVE_REMOTE:
            return storage_location_type::network_drive;
        case DRIVE_RAMDISK:
            return storage_location_type::ram;
        default:
            return storage_location_type::unknown;
        }
    }

    result<bool> is_network_path_impl(const path &p) noexcept {
        std::wstring path_str = p.wstring();

        // Check for UNC path
        if (path_str.size() >= 2 && path_str[0] == L'\\' && path_str[1] == L'\\') {
            return true;
        }

        // Check drive type
        auto volume_path = get_volume_path(p);
        if (!volume_path.empty()) {
            UINT drive_type = GetDriveTypeW(volume_path.c_str());
            return drive_type == DRIVE_REMOTE;
        }

        return false;
    }

    result<bool> is_unc_path_impl(const path &p) noexcept {
        std::wstring path_str = p.wstring();
        return path_str.size() >= 2 && path_str[0] == L'\\' && path_str[1] == L'\\';
    }

    result<path> get_unc_path_impl(const path &p) noexcept {
        std::wstring path_str = p.wstring();

        // Already UNC path
        if (path_str.size() >= 2 && path_str[0] == L'\\' && path_str[1] == L'\\') {
            return p;
        }

        // Try to get UNC path for mapped drive
        if (path_str.size() >= 2 && path_str[1] == L':') {
            wchar_t drive[3] = {path_str[0], L':', L'\0'};
            wchar_t remote_name[MAX_PATH] = {0};
            DWORD size = MAX_PATH;

            DWORD result = WNetGetConnectionW(drive, remote_name, &size);
            if (result == NO_ERROR) {
                std::wstring unc_path = remote_name;
                if (path_str.size() > 2) {
                    unc_path += path_str.substr(2);
                }
                return path(unc_path);
            }
        }

        return std::unexpected(make_error(std::errc::not_supported));
    }

    result<network_share_info> get_network_share_info_impl(const path &p) noexcept {
        network_share_info info;

        std::wstring path_str = p.wstring();

        // Parse UNC path or get UNC for mapped drive
        std::wstring unc_path;
        if (path_str.size() >= 2 && path_str[0] == L'\\' && path_str[1] == L'\\') {
            unc_path = path_str;
        } else if (path_str.size() >= 2 && path_str[1] == L':') {
            wchar_t drive[3] = {path_str[0], L':', L'\0'};
            wchar_t remote_name[MAX_PATH] = {0};
            DWORD size = MAX_PATH;

            if (WNetGetConnectionW(drive, remote_name, &size) == NO_ERROR) {
                unc_path = remote_name;
                info.local_mount_point = std::wstring(drive) + L"\\";
            } else {
                return std::unexpected(make_error(std::errc::not_supported));
            }
        } else {
            return std::unexpected(make_error(std::errc::invalid_argument));
        }

        // Parse UNC path: \\server\share\path
        if (unc_path.size() > 2) {
            std::size_t server_start = 2;
            std::size_t server_end = unc_path.find(L'\\', server_start);
            if (server_end != std::wstring::npos) {
                std::wstring server = unc_path.substr(server_start, server_end - server_start);
                info.server = std::string(server.begin(), server.end());

                std::size_t share_start = server_end + 1;
                std::size_t share_end = unc_path.find(L'\\', share_start);
                if (share_end == std::wstring::npos) {
                    share_end = unc_path.size();
                }

                std::wstring share = unc_path.substr(share_start, share_end - share_start);
                info.share_name = std::string(share.begin(), share.end());

                if (share_end < unc_path.size()) {
                    std::wstring remote = unc_path.substr(share_end);
                    info.remote_path = std::string(remote.begin(), remote.end());
                }
            }
        }

        info.protocol = network_protocol::smb;
        info.is_connected = true;

        return info;
    }

    result<std::vector<network_share_info>> list_mapped_drives_impl() noexcept;

    result<std::vector<network_share_info>> list_network_shares_impl() noexcept {
        return list_mapped_drives_impl();
    }

    result<std::vector<network_share_info>> list_mapped_drives_impl() noexcept {
        std::vector<network_share_info> shares;

        DWORD drives = GetLogicalDrives();
        if (drives == 0) {
            return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
        }

        for (int i = 0; i < 26; ++i) {
            if (drives & (1 << i)) {
                wchar_t drive[3] = {static_cast<wchar_t>(L'A' + i), L':', L'\0'};
                wchar_t drive_path[4] = {static_cast<wchar_t>(L'A' + i), L':', L'\\', L'\0'};

                if (GetDriveTypeW(drive_path) == DRIVE_REMOTE) {
                    wchar_t remote_name[MAX_PATH] = {0};
                    DWORD size = MAX_PATH;

                    if (WNetGetConnectionW(drive, remote_name, &size) == NO_ERROR) {
                        auto info = get_network_share_info_impl(remote_name);
                        if (info) {
                            info->local_mount_point = std::wstring(drive_path);
                            shares.push_back(std::move(*info));
                        }
                    }
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

        DWORD drives = GetLogicalDrives();
        for (int i = 0; i < 26; ++i) {
            if (!(drives & (1 << i))) {
                continue;
            }

            wchar_t drive_path[4] = {static_cast<wchar_t>(L'A' + i), L':', L'\\', L'\0'};
            UINT drive_type = GetDriveTypeW(drive_path);

            if (drive_type == DRIVE_REMOVABLE || drive_type == DRIVE_CDROM) {
                removable_device_info info;
                info.mount_point = std::wstring(drive_path);
                info.device_path = info.mount_point;
                info.is_mounted = true;

                if (drive_type == DRIVE_CDROM) {
                    info.device_type = removable_device_type::optical;
                } else {
                    info.device_type = removable_device_type::usb_drive;
                }

                wchar_t volume_name[MAX_PATH] = {0};
                wchar_t fs_name[MAX_PATH] = {0};
                DWORD serial = 0;

                if (GetVolumeInformationW(drive_path, volume_name, MAX_PATH, &serial, nullptr, nullptr, fs_name,
                                          MAX_PATH)) {
                    std::wstring wlabel(volume_name);
                    info.label = std::string(wlabel.begin(), wlabel.end());

                    std::wstring wfs(fs_name);
                    std::string fs_str(wfs.begin(), wfs.end());
                    if (fs_str == "NTFS") {
                        info.fs_type = filesystem_type::ntfs;
                    } else if (fs_str == "FAT32") {
                        info.fs_type = filesystem_type::fat32;
                    } else if (fs_str == "exFAT") {
                        info.fs_type = filesystem_type::exfat;
                    } else if (fs_str == "FAT") {
                        info.fs_type = filesystem_type::fat;
                    }
                }

                ULARGE_INTEGER total_bytes;
                if (GetDiskFreeSpaceExW(drive_path, nullptr, &total_bytes, nullptr)) {
                    info.size = total_bytes.QuadPart;
                }

                info.is_ejectable = true;
                info.bus_type = "USB";

                devices.push_back(info);
            }
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
        std::wstring root = p.root_path().wstring();
        UINT type = GetDriveTypeW(root.c_str());
        if (type == DRIVE_NO_ROOT_DIR) {
            return false;
        }

        ULARGE_INTEGER free_bytes;
        return GetDiskFreeSpaceExW(root.c_str(), &free_bytes, nullptr, nullptr) != 0;
    }

    result<bool> is_media_present_impl(const path &p) noexcept {
        return is_device_ready_impl(p);
    }

    // ============================================================================
    // 6.10 云存储挂载
    // ============================================================================

    result<bool> is_cloud_path_impl(const path &p) noexcept {
        std::string path_str = p.string();
        std::transform(path_str.begin(), path_str.end(), path_str.begin(), ::tolower);

        if (path_str.find("onedrive") != std::string::npos) {
            return true;
        }
        if (path_str.find("dropbox") != std::string::npos) {
            return true;
        }
        if (path_str.find("google drive") != std::string::npos) {
            return true;
        }
        if (path_str.find("icloud") != std::string::npos) {
            return true;
        }
        if (path_str.find("box sync") != std::string::npos) {
            return true;
        }

        return false;
    }

    result<cloud_provider> get_cloud_provider_impl(const path &p) noexcept {
        std::string path_str = p.string();
        std::transform(path_str.begin(), path_str.end(), path_str.begin(), ::tolower);

        if (path_str.find("onedrive") != std::string::npos) {
            return cloud_provider::onedrive;
        }
        if (path_str.find("dropbox") != std::string::npos) {
            return cloud_provider::dropbox;
        }
        if (path_str.find("google drive") != std::string::npos) {
            return cloud_provider::google_drive;
        }
        if (path_str.find("icloud") != std::string::npos) {
            return cloud_provider::icloud;
        }
        if (path_str.find("box sync") != std::string::npos) {
            return cloud_provider::box;
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

        info.mount_point = p.root_path();
        info.is_online = true;

        return info;
    }

    result<std::vector<cloud_storage_info>> list_cloud_mounts_impl() noexcept {
        std::vector<cloud_storage_info> mounts;

        // Check common cloud storage paths
        wchar_t user_profile[MAX_PATH];
        if (GetEnvironmentVariableW(L"USERPROFILE", user_profile, MAX_PATH)) {
            path user_path(user_profile);

            // OneDrive
            path onedrive = user_path / "OneDrive";
            if (std::filesystem::exists(onedrive)) {
                cloud_storage_info info;
                info.provider = cloud_provider::onedrive;
                info.mount_point = onedrive;
                info.is_online = true;
                mounts.push_back(info);
            }

            // Dropbox
            path dropbox = user_path / "Dropbox";
            if (std::filesystem::exists(dropbox)) {
                cloud_storage_info info;
                info.provider = cloud_provider::dropbox;
                info.mount_point = dropbox;
                info.is_online = true;
                mounts.push_back(info);
            }

            // Google Drive
            path gdrive = user_path / "Google Drive";
            if (std::filesystem::exists(gdrive)) {
                cloud_storage_info info;
                info.provider = cloud_provider::google_drive;
                info.mount_point = gdrive;
                info.is_online = true;
                mounts.push_back(info);
            }

            // iCloud
            path icloud = user_path / "iCloudDrive";
            if (std::filesystem::exists(icloud)) {
                cloud_storage_info info;
                info.provider = cloud_provider::icloud;
                info.mount_point = icloud;
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
        std::wstring root = p.root_path().wstring();

        wchar_t fs_name[MAX_PATH] = {0};
        if (!GetVolumeInformationW(root.c_str(), nullptr, 0, nullptr, nullptr, nullptr, fs_name, MAX_PATH)) {
            return false;
        }

        std::wstring fs(fs_name);
        if (fs == L"WinFsp" || fs == L"Dokan" || fs == L"FUSE") {
            return true;
        }

        return false;
    }

    result<bool> is_fuse_mount_impl(const path &p) noexcept {
        return is_virtual_filesystem_impl(p);
    }

    result<bool> is_encrypted_mount_impl(const path &p) noexcept {
        std::wstring root = p.root_path().wstring();

        wchar_t fs_name[MAX_PATH] = {0};
        DWORD flags = 0;
        if (!GetVolumeInformationW(root.c_str(), nullptr, 0, nullptr, nullptr, &flags, fs_name, MAX_PATH)) {
            return false;
        }

        return (flags & FILE_VOLUME_IS_COMPRESSED) != 0;
    }

    result<virtual_fs_info> get_virtual_fs_info_impl(const path &p) noexcept {
        virtual_fs_info info;
        info.mount_point = p.root_path();

        std::wstring root = p.root_path().wstring();
        wchar_t fs_name[MAX_PATH] = {0};
        if (GetVolumeInformationW(root.c_str(), nullptr, 0, nullptr, nullptr, nullptr, fs_name, MAX_PATH)) {
            std::wstring fs(fs_name);
            if (fs == L"WinFsp") {
                info.type = virtual_fs_type::winfsp;
            } else if (fs == L"Dokan") {
                info.type = virtual_fs_type::dokan;
            }
        }

        return info;
    }

    result<std::vector<virtual_fs_info>> list_virtual_mounts_impl() noexcept {
        std::vector<virtual_fs_info> mounts;

        DWORD drives = GetLogicalDrives();
        for (int i = 0; i < 26; ++i) {
            if (!(drives & (1 << i))) {
                continue;
            }

            wchar_t drive_path[4] = {static_cast<wchar_t>(L'A' + i), L':', L'\\', L'\0'};
            wchar_t fs_name[MAX_PATH] = {0};

            if (GetVolumeInformationW(drive_path, nullptr, 0, nullptr, nullptr, nullptr, fs_name, MAX_PATH)) {
                std::wstring fs(fs_name);
                if (fs == L"WinFsp" || fs == L"Dokan") {
                    virtual_fs_info info;
                    info.mount_point = drive_path;
                    info.type = (fs == L"WinFsp") ? virtual_fs_type::winfsp : virtual_fs_type::dokan;
                    mounts.push_back(info);
                }
            }
        }

        return mounts;
    }

    // ============================================================================
    // 6.12 Mount Point Management
    // ============================================================================

    namespace {
        storage_location_type get_storage_type_for_drive(UINT drive_type) {
            switch (drive_type) {
            case DRIVE_REMOVABLE:
                return storage_location_type::removable;
            case DRIVE_FIXED:
                return storage_location_type::local;
            case DRIVE_REMOTE:
                return storage_location_type::network_share;
            case DRIVE_CDROM:
                return storage_location_type::removable;
            case DRIVE_RAMDISK:
                return storage_location_type::ram;
            default:
                return storage_location_type::unknown;
            }
        }

        filesystem_type get_fs_type_from_name(const std::wstring &fs_name) {
            if (fs_name == L"NTFS") {
                return filesystem_type::ntfs;
            }
            if (fs_name == L"FAT32") {
                return filesystem_type::fat32;
            }
            if (fs_name == L"FAT") {
                return filesystem_type::fat;
            }
            if (fs_name == L"exFAT") {
                return filesystem_type::exfat;
            }
            if (fs_name == L"ReFS") {
                return filesystem_type::refs;
            }
            return filesystem_type::unknown;
        }
    } // namespace

    result<std::vector<mount_point_info>> list_all_mounts_impl() noexcept {
        std::vector<mount_point_info> mounts;

        DWORD drives = GetLogicalDrives();
        for (int i = 0; i < 26; ++i) {
            if (!(drives & (1 << i))) {
                continue;
            }

            wchar_t drive_path[4] = {static_cast<wchar_t>(L'A' + i), L':', L'\\', L'\0'};
            UINT drive_type = GetDriveTypeW(drive_path);

            mount_point_info info;
            info.mount_path = drive_path;
            info.storage_type = get_storage_type_for_drive(drive_type);

            wchar_t volume_name[MAX_PATH] = {0};
            wchar_t fs_name[MAX_PATH] = {0};
            DWORD flags = 0;

            if (GetVolumeInformationW(drive_path, volume_name, MAX_PATH, nullptr, nullptr, &flags, fs_name, MAX_PATH)) {
                info.fs_type = get_fs_type_from_name(fs_name);
                info.is_readonly = (flags & FILE_READ_ONLY_VOLUME) != 0;
            }

            // Get device name
            wchar_t device_name[MAX_PATH] = {0};
            wchar_t drive_letter[3] = {static_cast<wchar_t>(L'A' + i), L':', L'\0'};
            if (QueryDosDeviceW(drive_letter, device_name, MAX_PATH)) {
                std::wstring wdev(device_name);
                info.device = std::string(wdev.begin(), wdev.end());
            }

            // Check mount options
            if (drive_type == DRIVE_REMOTE) {
                info.options.push_back("network");
            }

            mounts.push_back(info);
        }

        // Also enumerate volume mount points
        wchar_t volume_name[MAX_PATH] = {0};
        HANDLE hFind = FindFirstVolumeW(volume_name, MAX_PATH);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                wchar_t mount_paths[MAX_PATH * 4] = {0};
                DWORD chars_returned = 0;
                if (GetVolumePathNamesForVolumeNameW(volume_name, mount_paths, MAX_PATH * 4, &chars_returned)) {
                    // Parse multiple null-terminated paths
                    wchar_t *current = mount_paths;
                    while (*current) {
                        std::wstring mp(current);
                        // Skip if already in list (drive letters)
                        bool found = false;
                        for (const auto &m : mounts) {
                            if (m.mount_path.wstring() == mp) {
                                found = true;
                                break;
                            }
                        }
                        if (!found && mp.length() > 3) { // Not a drive root
                            mount_point_info info;
                            info.mount_path = mp;
                            info.device = std::string(volume_name, volume_name + wcslen(volume_name));
                            info.storage_type = storage_location_type::local;

                            wchar_t fs_name[MAX_PATH] = {0};
                            if (GetVolumeInformationW(volume_name, nullptr, 0, nullptr, nullptr, nullptr, fs_name,
                                                      MAX_PATH)) {
                                info.fs_type = get_fs_type_from_name(fs_name);
                            }

                            mounts.push_back(info);
                        }
                        current += wcslen(current) + 1;
                    }
                }
            } while (FindNextVolumeW(hFind, volume_name, MAX_PATH));
            FindVolumeClose(hFind);
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

        // Find best matching mount point
        mount_point_info best_match;
        size_t best_len = 0;

        for (const auto &m : *all_mounts) {
            std::string mount_str = m.mount_path.string();
            std::string path_str = abs_path.string();

            if (path_str.starts_with(mount_str) && mount_str.length() > best_len) {
                best_len = mount_str.length();
                best_match = m;
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

        for (const auto &m : *all_mounts) {
            if (std::filesystem::equivalent(m.mount_path, abs_path, ec)) {
                return true;
            }
        }
        return false;
    }

} // namespace frappe::detail

#endif // FRAPPE_PLATFORM_WINDOWS
