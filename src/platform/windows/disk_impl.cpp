#ifdef FRAPPE_PLATFORM_WINDOWS

#define NOMINMAX
#include "frappe/disk.hpp"

#include <Windows.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <ntddscsi.h>
#include <setupapi.h>
#include <winioctl.h>

#pragma comment(lib, "setupapi.lib")

namespace frappe::detail {

namespace {
std::string wstring_to_string(const std::wstring &wstr) {
    if (wstr.empty()) {
        return "";
    }
    return std::string(wstr.begin(), wstr.end());
}

partition_type get_partition_type_from_guid(const GUID &guid) {
    // EFI System Partition
    static const GUID EFI_SYSTEM = {0xC12A7328, 0xF81F, 0x11D2, {0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B}};
    // Microsoft Basic Data
    static const GUID MS_BASIC_DATA = {0xEBD0A0A2, 0xB9E5, 0x4433, {0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7}};
    // Microsoft Reserved
    static const GUID MS_RESERVED = {0xE3C9E316, 0x0B5C, 0x4DB8, {0x81, 0x7D, 0xF9, 0x2D, 0xF0, 0x02, 0x15, 0xAE}};
    // Microsoft Recovery
    static const GUID MS_RECOVERY = {0xDE94BBA4, 0x06D1, 0x4D40, {0xA1, 0x6A, 0xBF, 0xD5, 0x01, 0x79, 0xD6, 0xAC}};
    // Linux Filesystem
    static const GUID LINUX_FS = {0x0FC63DAF, 0x8483, 0x4772, {0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4}};
    // Linux Swap
    static const GUID LINUX_SWAP = {0x0657FD6D, 0xA4AB, 0x43C4, {0x84, 0xE5, 0x09, 0x33, 0xC8, 0x4B, 0x4F, 0x4F}};

    if (memcmp(&guid, &EFI_SYSTEM, sizeof(GUID)) == 0) {
        return partition_type::efi_system;
    }
    if (memcmp(&guid, &MS_BASIC_DATA, sizeof(GUID)) == 0) {
        return partition_type::microsoft_basic_data;
    }
    if (memcmp(&guid, &MS_RESERVED, sizeof(GUID)) == 0) {
        return partition_type::microsoft_reserved;
    }
    if (memcmp(&guid, &MS_RECOVERY, sizeof(GUID)) == 0) {
        return partition_type::microsoft_recovery;
    }
    if (memcmp(&guid, &LINUX_FS, sizeof(GUID)) == 0) {
        return partition_type::linux_filesystem;
    }
    if (memcmp(&guid, &LINUX_SWAP, sizeof(GUID)) == 0) {
        return partition_type::linux_swap;
    }

    return partition_type::unknown;
}

std::string guid_to_string(const GUID &guid) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guid.Data1, guid.Data2,
             guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
             guid.Data4[6], guid.Data4[7]);
    return buffer;
}

disk_bus_type get_bus_type(STORAGE_BUS_TYPE bus) {
    switch (bus) {
    case BusTypeAta:
        return disk_bus_type::ata;
    case BusTypeSata:
        return disk_bus_type::sata;
    case BusTypeScsi:
        return disk_bus_type::scsi;
    case BusTypeSas:
        return disk_bus_type::sas;
    case BusTypeUsb:
        return disk_bus_type::usb;
    case BusTypeNvme:
        return disk_bus_type::nvme;
    case BusTypeRAID:
        return disk_bus_type::raid;
    case BusTypeMmc:
        return disk_bus_type::mmc;
    case BusTypeVirtual:
        return disk_bus_type::virtual_bus;
    default:
        return disk_bus_type::unknown;
    }
}
} // namespace

result<std::vector<disk_info>> list_disks_impl() noexcept {
    std::vector<disk_info> disks;

    for (int i = 0; i < 32; ++i) {
        std::wstring path = L"\\\\.\\PhysicalDrive" + std::to_wstring(i);

        HANDLE hDisk =
            CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

        if (hDisk == INVALID_HANDLE_VALUE) {
            continue;
        }

        disk_info info;
        info.device_path = "\\\\.\\PhysicalDrive" + std::to_string(i);

        // Get disk geometry
        DISK_GEOMETRY_EX geometry;
        DWORD bytes_returned;
        if (DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, nullptr, 0, &geometry, sizeof(geometry),
                            &bytes_returned, nullptr)) {
            info.size = geometry.DiskSize.QuadPart;
            info.sector_size = geometry.Geometry.BytesPerSector;
            info.total_sectors = info.size / info.sector_size;
        }

        // Get storage device descriptor
        STORAGE_PROPERTY_QUERY query = {};
        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;

        BYTE buffer[4096];
        if (DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), buffer, sizeof(buffer),
                            &bytes_returned, nullptr)) {
            STORAGE_DEVICE_DESCRIPTOR *desc = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR *>(buffer);

            if (desc->VendorIdOffset) {
                info.vendor = reinterpret_cast<char *>(buffer + desc->VendorIdOffset);
            }
            if (desc->ProductIdOffset) {
                info.model = reinterpret_cast<char *>(buffer + desc->ProductIdOffset);
            }
            if (desc->SerialNumberOffset) {
                info.serial = reinterpret_cast<char *>(buffer + desc->SerialNumberOffset);
            }
            if (desc->ProductRevisionOffset) {
                info.firmware_version = reinterpret_cast<char *>(buffer + desc->ProductRevisionOffset);
            }

            info.is_removable = desc->RemovableMedia != FALSE;
            info.bus_type = get_bus_type(desc->BusType);

            // Determine disk type based on bus type
            if (info.bus_type == disk_bus_type::nvme) {
                info.type = disk_type::nvme;
            } else if (info.bus_type == disk_bus_type::usb) {
                info.type = disk_type::usb;
            } else {
                // Try to detect SSD vs HDD
                STORAGE_PROPERTY_QUERY seek_query = {};
                seek_query.PropertyId = StorageDeviceSeekPenaltyProperty;
                seek_query.QueryType = PropertyStandardQuery;

                DEVICE_SEEK_PENALTY_DESCRIPTOR seek_desc;
                if (DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &seek_query, sizeof(seek_query), &seek_desc,
                                    sizeof(seek_desc), &bytes_returned, nullptr)) {
                    info.type = seek_desc.IncursSeekPenalty ? disk_type::hdd : disk_type::ssd;
                } else {
                    info.type = disk_type::hdd;
                }
            }
        }

        // Get partition layout
        BYTE layout_buffer[16384];
        if (DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, nullptr, 0, layout_buffer, sizeof(layout_buffer),
                            &bytes_returned, nullptr)) {
            DRIVE_LAYOUT_INFORMATION_EX *layout = reinterpret_cast<DRIVE_LAYOUT_INFORMATION_EX *>(layout_buffer);

            if (layout->PartitionStyle == PARTITION_STYLE_GPT) {
                info.partition_table = partition_table_type::gpt;
            } else if (layout->PartitionStyle == PARTITION_STYLE_MBR) {
                info.partition_table = partition_table_type::mbr;
            }

            info.partition_count = layout->PartitionCount;
        }

        CloseHandle(hDisk);
        disks.push_back(info);
    }

    return disks;
}

result<disk_info> get_disk_info_impl(std::string_view device_path) noexcept {
    auto disks = list_disks_impl();
    if (!disks) {
        return std::unexpected(disks.error());
    }

    for (const auto &d : *disks) {
        if (d.device_path == device_path) {
            return d;
        }
    }

    return std::unexpected(make_error(std::errc::no_such_device));
}

result<disk_info> get_disk_for_path_impl(const path &p) noexcept {
    std::wstring vol_path = p.root_name().wstring() + L"\\";

    wchar_t volume_name[MAX_PATH] = {0};
    if (!GetVolumeNameForVolumeMountPointW(vol_path.c_str(), volume_name, MAX_PATH)) {
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    // Remove trailing backslash for CreateFile
    std::wstring vol_str(volume_name);
    if (!vol_str.empty() && vol_str.back() == L'\\') {
        vol_str.pop_back();
    }

    HANDLE hVol =
        CreateFileW(vol_str.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hVol == INVALID_HANDLE_VALUE) {
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    VOLUME_DISK_EXTENTS extents;
    DWORD bytes_returned;
    if (!DeviceIoControl(hVol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, &extents, sizeof(extents),
                         &bytes_returned, nullptr)) {
        CloseHandle(hVol);
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    CloseHandle(hVol);

    if (extents.NumberOfDiskExtents > 0) {
        std::string device = "\\\\.\\PhysicalDrive" + std::to_string(extents.Extents[0].DiskNumber);
        return get_disk_info_impl(device);
    }

    return std::unexpected(make_error(std::errc::no_such_device));
}

result<partition_table_info> get_partition_table_impl(std::string_view device_path) noexcept {
    partition_table_info info;
    info.device_path = std::string(device_path);

    std::wstring wpath(device_path.begin(), device_path.end());

    HANDLE hDisk =
        CreateFileW(wpath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hDisk == INVALID_HANDLE_VALUE) {
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    // Get disk geometry
    DISK_GEOMETRY_EX geometry;
    DWORD bytes_returned;
    if (DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, nullptr, 0, &geometry, sizeof(geometry),
                        &bytes_returned, nullptr)) {
        info.disk_size = geometry.DiskSize.QuadPart;
        info.sector_size = geometry.Geometry.BytesPerSector;
        info.total_sectors = info.disk_size / info.sector_size;
    }

    // Get partition layout
    BYTE buffer[32768];
    if (!DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, nullptr, 0, buffer, sizeof(buffer), &bytes_returned,
                         nullptr)) {
        CloseHandle(hDisk);
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    DRIVE_LAYOUT_INFORMATION_EX *layout = reinterpret_cast<DRIVE_LAYOUT_INFORMATION_EX *>(buffer);

    if (layout->PartitionStyle == PARTITION_STYLE_GPT) {
        info.type = partition_table_type::gpt;
        info.disk_uuid = guid_to_string(layout->Gpt.DiskId);
        info.first_usable_sector = layout->Gpt.StartingUsableOffset.QuadPart / info.sector_size;
        info.last_usable_sector = layout->Gpt.UsableLength.QuadPart / info.sector_size + info.first_usable_sector;
    } else if (layout->PartitionStyle == PARTITION_STYLE_MBR) {
        info.type = partition_table_type::mbr;
        info.disk_signature = layout->Mbr.Signature;
        info.first_usable_sector = 1;
        info.last_usable_sector = info.total_sectors - 1;
    }

    for (DWORD i = 0; i < layout->PartitionCount; ++i) {
        const PARTITION_INFORMATION_EX &part = layout->PartitionEntry[i];

        if (part.PartitionLength.QuadPart == 0) {
            continue;
        }

        partition_info pinfo;
        pinfo.device_path = std::string(device_path);
        pinfo.partition_number = part.PartitionNumber;
        pinfo.start_sector = part.StartingOffset.QuadPart / info.sector_size;
        pinfo.size = part.PartitionLength.QuadPart;
        pinfo.sector_count = pinfo.size / info.sector_size;
        pinfo.end_sector = pinfo.start_sector + pinfo.sector_count - 1;
        pinfo.offset = part.StartingOffset.QuadPart;

        if (layout->PartitionStyle == PARTITION_STYLE_GPT) {
            pinfo.type_uuid = guid_to_string(part.Gpt.PartitionType);
            pinfo.uuid = guid_to_string(part.Gpt.PartitionId);
            pinfo.type = get_partition_type_from_guid(part.Gpt.PartitionType);

            std::wstring wname(part.Gpt.Name);
            pinfo.label = wstring_to_string(wname);
        } else if (layout->PartitionStyle == PARTITION_STYLE_MBR) {
            pinfo.type_id = part.Mbr.PartitionType;
            pinfo.is_bootable = part.Mbr.BootIndicator != FALSE;
            pinfo.is_primary =
                !part.Mbr.RecognizedPartition || (part.Mbr.PartitionType != 0x05 && part.Mbr.PartitionType != 0x0F);
            pinfo.is_extended = part.Mbr.PartitionType == 0x05 || part.Mbr.PartitionType == 0x0F;

            if (part.Mbr.PartitionType == 0x07) {
                pinfo.type = partition_type::microsoft_basic_data;
            } else if (part.Mbr.PartitionType == 0x83) {
                pinfo.type = partition_type::linux_filesystem;
            } else if (part.Mbr.PartitionType == 0x82) {
                pinfo.type = partition_type::linux_swap;
            }
        }

        info.partitions.push_back(pinfo);
    }

    CloseHandle(hDisk);
    return info;
}

result<partition_info> get_partition_info_impl(const path &p) noexcept {
    auto disk = get_disk_for_path_impl(p);
    if (!disk) {
        return std::unexpected(disk.error());
    }

    auto table = get_partition_table_impl(disk->device_path);
    if (!table) {
        return std::unexpected(table.error());
    }

    std::wstring vol_path = p.root_name().wstring() + L"\\";

    wchar_t volume_name[MAX_PATH] = {0};
    if (!GetVolumeNameForVolumeMountPointW(vol_path.c_str(), volume_name, MAX_PATH)) {
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    std::wstring vol_str(volume_name);
    if (!vol_str.empty() && vol_str.back() == L'\\') {
        vol_str.pop_back();
    }

    HANDLE hVol =
        CreateFileW(vol_str.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hVol == INVALID_HANDLE_VALUE) {
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    VOLUME_DISK_EXTENTS extents;
    DWORD bytes_returned;
    if (!DeviceIoControl(hVol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, &extents, sizeof(extents),
                         &bytes_returned, nullptr)) {
        CloseHandle(hVol);
        return std::unexpected(make_error(static_cast<int>(GetLastError()), std::system_category()));
    }

    CloseHandle(hVol);

    if (extents.NumberOfDiskExtents > 0) {
        std::uint64_t offset = extents.Extents[0].StartingOffset.QuadPart;

        for (auto &part : table->partitions) {
            if (part.offset == offset) {
                part.mount_point = p.root_name().string() + "\\";
                return part;
            }
        }
    }

    return std::unexpected(make_error(std::errc::no_such_device));
}

result<std::vector<partition_info>> list_partitions_impl() noexcept {
    std::vector<partition_info> all_partitions;

    auto disks = list_disks_impl();
    if (!disks) {
        return std::unexpected(disks.error());
    }

    for (const auto &disk : *disks) {
        auto table = get_partition_table_impl(disk.device_path);
        if (table) {
            for (auto &part : table->partitions) {
                all_partitions.push_back(part);
            }
        }
    }

    // Try to find mount points for partitions
    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; ++i) {
        if (!(drives & (1 << i))) {
            continue;
        }

        wchar_t drive_letter = static_cast<wchar_t>(L'A' + i);
        std::wstring drive_path = std::wstring(1, drive_letter) + L":\\";

        wchar_t volume_name[MAX_PATH] = {0};
        if (!GetVolumeNameForVolumeMountPointW(drive_path.c_str(), volume_name, MAX_PATH)) {
            continue;
        }

        std::wstring vol_str(volume_name);
        if (!vol_str.empty() && vol_str.back() == L'\\') {
            vol_str.pop_back();
        }

        HANDLE hVol =
            CreateFileW(vol_str.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hVol == INVALID_HANDLE_VALUE) {
            continue;
        }

        VOLUME_DISK_EXTENTS extents;
        DWORD bytes_returned;
        if (DeviceIoControl(hVol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, &extents, sizeof(extents),
                            &bytes_returned, nullptr)) {
            if (extents.NumberOfDiskExtents > 0) {
                std::uint64_t offset = extents.Extents[0].StartingOffset.QuadPart;

                for (auto &part : all_partitions) {
                    if (part.offset == offset) {
                        part.mount_point = std::string(1, static_cast<char>(drive_letter)) + ":\\";
                        break;
                    }
                }
            }
        }

        CloseHandle(hVol);
    }

    return all_partitions;
}

result<std::string> get_containing_device_impl(const path &p) noexcept {
    auto disk = get_disk_for_path_impl(p);
    if (!disk) {
        return std::unexpected(disk.error());
    }
    return disk->device_path;
}

// ============================================================================
// 7.12 Mount Information
// ============================================================================

result<std::vector<mount_info>> list_mounts_impl() noexcept {
    std::vector<mount_info> mounts;

    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; ++i) {
        if (!(drives & (1 << i))) {
            continue;
        }

        wchar_t drive_letter = static_cast<wchar_t>(L'A' + i);
        std::wstring drive_path = std::wstring(1, drive_letter) + L":\\";

        mount_info info;
        info.mount_point = std::string(1, static_cast<char>(drive_letter)) + ":\\";

        // Get drive type
        UINT drive_type = GetDriveTypeW(drive_path.c_str());
        info.is_network = (drive_type == DRIVE_REMOTE);
        info.is_removable = (drive_type == DRIVE_REMOVABLE || drive_type == DRIVE_CDROM);

        // Get volume information
        wchar_t volume_name[MAX_PATH] = {0};
        wchar_t fs_name[MAX_PATH] = {0};
        DWORD serial = 0;
        DWORD max_component = 0;
        DWORD flags = 0;

        if (GetVolumeInformationW(drive_path.c_str(), volume_name, MAX_PATH, &serial, &max_component, &flags, fs_name,
                                  MAX_PATH)) {
            std::wstring wfs(fs_name);
            info.fs_type_name = std::string(wfs.begin(), wfs.end());

            if (info.fs_type_name == "NTFS") {
                info.fs_type = filesystem_type::ntfs;
            } else if (info.fs_type_name == "FAT32") {
                info.fs_type = filesystem_type::fat32;
            } else if (info.fs_type_name == "exFAT") {
                info.fs_type = filesystem_type::exfat;
            } else if (info.fs_type_name == "ReFS") {
                info.fs_type = filesystem_type::refs;
            }

            info.is_readonly = (flags & FILE_READ_ONLY_VOLUME) != 0;
        }

        // Get device path
        wchar_t volume_path[MAX_PATH] = {0};
        if (GetVolumeNameForVolumeMountPointW(drive_path.c_str(), volume_path, MAX_PATH)) {
            std::wstring wvol(volume_path);
            info.device = std::string(wvol.begin(), wvol.end());
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

    std::wstring wpath(device_path.begin(), device_path.end());
    HANDLE hDisk = CreateFileW(wpath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                               OPEN_EXISTING, 0, nullptr);

    if (hDisk == INVALID_HANDLE_VALUE) {
        // Try read-only access
        hDisk = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0,
                            nullptr);
        if (hDisk == INVALID_HANDLE_VALUE) {
            return std::unexpected(make_error(std::errc::permission_denied));
        }
    }

    // Query SMART data using IOCTL_STORAGE_QUERY_PROPERTY
    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    STORAGE_DEVICE_DESCRIPTOR desc = {};
    DWORD bytes_returned = 0;

    if (DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &desc, sizeof(desc),
                        &bytes_returned, nullptr)) {
        info.is_supported = true;
        info.is_enabled = true;
        info.health_status = smart_status::good;
    }

    CloseHandle(hDisk);
    return info;
}

// ============================================================================
// 7.10 NVMe Information
// ============================================================================

result<nvme_info> get_nvme_info_impl(std::string_view device_path) noexcept {
    nvme_info info;

    std::wstring wpath(device_path.begin(), device_path.end());
    HANDLE hDisk = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                               0, nullptr);

    if (hDisk == INVALID_HANDLE_VALUE) {
        return std::unexpected(make_error(std::errc::permission_denied));
    }

    // Query NVMe specific data
    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = StorageAdapterProtocolSpecificProperty;
    query.QueryType = PropertyStandardQuery;

    // Basic info - full NVMe query requires more complex setup
    info.namespace_id = 1;

    CloseHandle(hDisk);
    return info;
}

// ============================================================================
// 7.11 Storage Pool / RAID Information
// ============================================================================

result<std::vector<storage_pool_info>> list_storage_pools_impl() noexcept {
    std::vector<storage_pool_info> pools;
    // Windows Storage Spaces detection would require WMI or Storage Management API
    // Return empty list for basic implementation
    return pools;
}

result<std::vector<raid_info>> list_raid_arrays_impl() noexcept {
    std::vector<raid_info> arrays;
    // Windows RAID detection would require WMI
    // Return empty list for basic implementation
    return arrays;
}

// ============================================================================
// 7.13 Virtual Disk Support
// ============================================================================

namespace {
void parse_vhd_footer(const path &p, virtual_disk_info &info) {
    std::wstring wpath = p.wstring();
    HANDLE hFile = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }

    LARGE_INTEGER pos;
    pos.QuadPart = -512; // VHD footer is 512 bytes at end
    if (!SetFilePointerEx(hFile, pos, nullptr, FILE_END)) {
        CloseHandle(hFile);
        return;
    }

    char footer[512];
    DWORD bytes_read;
    if (!ReadFile(hFile, footer, 512, &bytes_read, nullptr) || bytes_read != 512) {
        CloseHandle(hFile);
        return;
    }
    CloseHandle(hFile);

    // VHD footer: magic "conectix", bytes 48-55 contain current size (big-endian)
    if (memcmp(footer, "conectix", 8) != 0) {
        return;
    }

    uint64_t size = 0;
    for (int i = 0; i < 8; ++i) {
        size = (size << 8) | static_cast<uint8_t>(footer[48 + i]);
    }
    info.virtual_size = size;
    info.is_dynamic = (footer[60] == 3); // Dynamic disk type
}
} // namespace

result<virtual_disk_info> get_virtual_disk_info_impl(const path &p) noexcept {
    virtual_disk_info info;
    info.file_path = p;

    std::error_code ec;
    if (!std::filesystem::exists(p, ec)) {
        return std::unexpected(make_error(std::errc::no_such_file_or_directory));
    }

    // Get file size
    info.actual_size = std::filesystem::file_size(p, ec);
    if (ec) {
        return std::unexpected(make_error(ec.value(), ec.category()));
    }

    // Determine type from extension
    auto ext = p.extension().string();
    for (auto &c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    if (ext == ".vhd") {
        info.type = virtual_disk_type::vhd;
    } else if (ext == ".vhdx") {
        info.type = virtual_disk_type::vhdx;
    } else if (ext == ".vmdk") {
        info.type = virtual_disk_type::vmdk;
    } else if (ext == ".vdi") {
        info.type = virtual_disk_type::vdi;
    } else if (ext == ".qcow" || ext == ".qcow2") {
        info.type = virtual_disk_type::qcow2;
    } else if (ext == ".iso") {
        info.type = virtual_disk_type::iso;
    } else if (ext == ".img") {
        info.type = virtual_disk_type::img;
    } else {
        info.type = virtual_disk_type::unknown;
    }

    // For VHD, try to read footer to get virtual size
    if (info.type == virtual_disk_type::vhd) {
        parse_vhd_footer(p, info);
    }

    if (info.virtual_size == 0) {
        info.virtual_size = info.actual_size;
    }

    // Get file times
    info.modification_time = std::filesystem::last_write_time(p, ec);

    return info;
}

result<std::vector<virtual_disk_info>> list_mounted_virtual_disks_impl() noexcept {
    std::vector<virtual_disk_info> disks;
    // Would require querying mounted VHD/VHDX via Virtual Disk API
    return disks;
}

} // namespace frappe::detail

#endif // FRAPPE_PLATFORM_WINDOWS
