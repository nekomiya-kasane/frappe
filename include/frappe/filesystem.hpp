#ifndef FRAPPE_FILESYSTEM_HPP
#define FRAPPE_FILESYSTEM_HPP

#include "frappe/error.hpp"
#include "frappe/exports.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace frappe {

using path = std::filesystem::path;

// ============================================================================
// 6.1 Disk Space (compatible with std::filesystem)
// ============================================================================

using space_info = std::filesystem::space_info;

/**
 * @brief Get disk space information for a path
 * @param p Path to query
 * @return Space information including capacity, free, and available space
 */
[[nodiscard]] FRAPPE_API result<space_info> space(const path &p) noexcept;

// ============================================================================
// 6.2 Filesystem Type Enumeration
// ============================================================================

/**
 * @brief Enumeration of supported filesystem types
 */
enum class filesystem_type {
    unknown,
    // Windows
    ntfs,  ///< NT File System
    fat,   ///< FAT12/FAT16
    fat32, ///< FAT32
    exfat, ///< Extended FAT
    refs,  ///< Resilient File System
    // Linux
    ext2,  ///< Second Extended Filesystem
    ext3,  ///< Third Extended Filesystem
    ext4,  ///< Fourth Extended Filesystem
    xfs,   ///< XFS Filesystem
    btrfs, ///< B-tree Filesystem
    zfs,   ///< Zettabyte File System
    // macOS
    apfs,     ///< Apple File System
    hfs_plus, ///< HFS+
    // Network
    nfs,    ///< Network File System
    smb,    ///< Server Message Block
    cifs,   ///< Common Internet File System
    webdav, ///< Web Distributed Authoring and Versioning
    // Special
    tmpfs,  ///< Temporary Filesystem
    ramfs,  ///< RAM Filesystem
    devfs,  ///< Device Filesystem
    procfs, ///< Process Filesystem
    sysfs,  ///< System Filesystem
    fuse,   ///< Filesystem in Userspace
    overlay ///< Overlay Filesystem
};

/**
 * @brief Get the string name of a filesystem type
 * @param type The filesystem type
 * @return String view of the filesystem type name
 */
[[nodiscard]] FRAPPE_API std::string_view filesystem_type_name(filesystem_type type) noexcept;

// ============================================================================
// 6.3 Filesystem Features Structure
// ============================================================================

/**
 * @brief Structure describing filesystem capabilities and features
 */
struct filesystem_features {
    filesystem_type type = filesystem_type::unknown; ///< Filesystem type
    bool case_sensitive = false;                     ///< True if filenames are case-sensitive
    bool case_preserving = true;                     ///< True if case is preserved in filenames
    bool supports_symlinks = false;                  ///< True if symbolic links are supported
    bool supports_hard_links = false;                ///< True if hard links are supported
    bool supports_permissions = false;               ///< True if POSIX permissions are supported
    bool supports_acl = false;                       ///< True if ACLs are supported
    bool supports_extended_attributes = false;       ///< True if extended attributes are supported
    bool supports_sparse_files = false;              ///< True if sparse files are supported
    bool supports_compression = false;               ///< True if filesystem compression is supported
    bool supports_encryption = false;                ///< True if filesystem encryption is supported
    bool supports_transactions = false;              ///< True if transactional operations are supported
    bool supports_quotas = false;                    ///< True if disk quotas are supported
    std::uint32_t max_filename_length = 255;         ///< Maximum filename length in characters
    std::uint32_t max_path_length = 4096;            ///< Maximum path length in characters
    std::uint32_t block_size = 0;                    ///< Filesystem block size in bytes
    std::uint32_t cluster_size = 0;                  ///< Filesystem cluster size in bytes
};

/**
 * @brief Get the filesystem type for a path
 * @param p Path to query
 * @return The filesystem type
 */
[[nodiscard]] FRAPPE_API result<filesystem_type> get_filesystem_type(const path &p) noexcept;

/**
 * @brief Get the filesystem features for a path
 * @param p Path to query
 * @return Structure containing filesystem capabilities
 */
[[nodiscard]] FRAPPE_API result<filesystem_features> get_filesystem_features(const path &p) noexcept;

// ============================================================================
// 6.5 Volume Information
// ============================================================================

/**
 * @brief Structure containing volume/partition information
 */
struct volume_info {
    std::string name;                                   ///< Volume name
    std::string label;                                  ///< Volume label
    std::string serial_number;                          ///< Volume serial number
    std::string uuid;                                   ///< Volume UUID
    path mount_point;                                   ///< Mount point path
    path device_path;                                   ///< Device path (e.g., /dev/sda1)
    filesystem_type fs_type = filesystem_type::unknown; ///< Filesystem type
    std::uintmax_t total_size = 0;                      ///< Total size in bytes
    std::uintmax_t free_size = 0;                       ///< Free space in bytes
    std::uintmax_t available_size = 0; ///< Available space in bytes (may differ from free due to quotas)
};

/**
 * @brief Get volume information for a path
 * @param p Path on the volume to query
 * @return Volume information structure
 */
[[nodiscard]] FRAPPE_API result<volume_info> get_volume_info(const path &p) noexcept;

/**
 * @brief List all mounted volumes
 * @return Vector of volume information for all mounted volumes
 */
[[nodiscard]] FRAPPE_API result<std::vector<volume_info>> list_volumes() noexcept;

// ============================================================================
// 6.6 Storage Location Type Enumeration
// ============================================================================

/**
 * @brief Enumeration of storage location types
 */
enum class storage_location_type {
    unknown,       ///< Unknown storage type
    local,         ///< Local fixed storage (HDD, SSD)
    removable,     ///< Removable storage (USB, SD card)
    optical,       ///< Optical drive (CD, DVD, Blu-ray)
    network_share, ///< Network share (SMB, NFS)
    network_drive, ///< Mapped network drive
    nfs,           ///< NFS mount
    cloud,         ///< Cloud storage mount
    virtual_fs,    ///< Virtual filesystem
    fuse,          ///< FUSE mount
    loop,          ///< Loop device
    ram            ///< RAM-based storage
};

/**
 * @brief Get the string name of a storage location type
 * @param type The storage location type
 * @return String view of the storage location type name
 */
[[nodiscard]] FRAPPE_API std::string_view storage_location_type_name(storage_location_type type) noexcept;

/**
 * @brief Get the storage location type for a path
 * @param p Path to query
 * @return The storage location type
 */
[[nodiscard]] FRAPPE_API result<storage_location_type> get_storage_location_type(const path &p) noexcept;

/**
 * @brief Check if a path is on local storage
 * @param p Path to check
 * @return True if on local storage
 */
[[nodiscard]] FRAPPE_API result<bool> is_local_storage(const path &p) noexcept;

/**
 * @brief Check if a path is on removable storage
 * @param p Path to check
 * @return True if on removable storage
 */
[[nodiscard]] FRAPPE_API result<bool> is_removable_storage(const path &p) noexcept;

/**
 * @brief Check if a path is on network storage
 * @param p Path to check
 * @return True if on network storage
 */
[[nodiscard]] FRAPPE_API result<bool> is_network_storage(const path &p) noexcept;

/**
 * @brief Check if a path is on virtual storage
 * @param p Path to check
 * @return True if on virtual storage
 */
[[nodiscard]] FRAPPE_API result<bool> is_virtual_storage(const path &p) noexcept;

// ============================================================================
// 6.8 Network Filesystem
// ============================================================================

/**
 * @brief Enumeration of network protocols
 */
enum class network_protocol {
    unknown, ///< Unknown protocol
    smb,     ///< SMB (generic)
    smb1,    ///< SMB version 1
    smb2,    ///< SMB version 2
    smb3,    ///< SMB version 3
    nfs,     ///< NFS (generic)
    nfs3,    ///< NFS version 3
    nfs4,    ///< NFS version 4
    webdav,  ///< WebDAV
    ftp,     ///< FTP
    sftp,    ///< SFTP (SSH File Transfer Protocol)
    afp      ///< Apple Filing Protocol
};

/**
 * @brief Get the string name of a network protocol
 * @param proto The network protocol
 * @return String view of the protocol name
 */
[[nodiscard]] FRAPPE_API std::string_view network_protocol_name(network_protocol proto) noexcept;

/**
 * @brief Structure containing network share information
 */
struct network_share_info {
    network_protocol protocol = network_protocol::unknown; ///< Network protocol used
    std::string server;                                    ///< Server hostname or IP
    std::uint16_t port = 0;                                ///< Port number (0 = default)
    std::string share_name;                                ///< Share name
    std::string remote_path;                               ///< Remote path on the share
    path local_mount_point;                                ///< Local mount point
    std::string username;                                  ///< Username for authentication
    std::string domain;                                    ///< Domain for authentication
    bool is_connected = false;                             ///< True if currently connected
    bool is_persistent = false;                            ///< True if connection persists across reboots
};

/**
 * @brief Check if a path is a network path
 * @param p Path to check
 * @return True if the path is on a network location
 */
[[nodiscard]] FRAPPE_API result<bool> is_network_path(const path &p) noexcept;

/**
 * @brief Check if a path is on a remote filesystem
 * @param p Path to check
 * @return True if the filesystem is remote
 */
[[nodiscard]] FRAPPE_API result<bool> is_remote_filesystem(const path &p) noexcept;

/**
 * @brief Check if a path is a UNC path
 * @param p Path to check
 * @return True if the path is in UNC format
 */
[[nodiscard]] FRAPPE_API result<bool> is_unc_path(const path &p) noexcept;

/**
 * @brief Convert a path to UNC format
 * @param p Path to convert
 * @return UNC path
 */
[[nodiscard]] FRAPPE_API result<path> get_unc_path(const path &p) noexcept;

/**
 * @brief Structure containing parsed UNC path components
 */
struct unc_path_components {
    std::string server; ///< Server name
    std::string share;  ///< Share name
    std::string path;   ///< Path within the share
};

/**
 * @brief Parse a UNC path into its components
 * @param p UNC path to parse
 * @return Parsed components, or nullopt if not a valid UNC path
 */
[[nodiscard]] FRAPPE_API std::optional<unc_path_components> parse_unc_path(const path &p) noexcept;

/**
 * @brief Get network share information for a path
 * @param p Path on the network share
 * @return Network share information
 */
[[nodiscard]] FRAPPE_API result<network_share_info> get_network_share_info(const path &p) noexcept;

/**
 * @brief List all network shares
 * @return Vector of network share information
 */
[[nodiscard]] FRAPPE_API result<std::vector<network_share_info>> list_network_shares() noexcept;

/**
 * @brief List all SMB shares
 * @return Vector of SMB share information
 */
[[nodiscard]] FRAPPE_API result<std::vector<network_share_info>> list_smb_shares() noexcept;

/**
 * @brief List all NFS mounts
 * @return Vector of NFS mount information
 */
[[nodiscard]] FRAPPE_API result<std::vector<network_share_info>> list_nfs_mounts() noexcept;

/**
 * @brief Check if a network share is available
 * @param p Path on the share to check
 * @return True if the share is available
 */
[[nodiscard]] FRAPPE_API result<bool> is_share_available(const path &p) noexcept;

/**
 * @brief Check if a network share is currently connected
 * @param p Path on the share to check
 * @return True if the share is connected
 */
[[nodiscard]] FRAPPE_API result<bool> is_share_connected(const path &p) noexcept;

/**
 * @brief Structure containing server information
 */
struct server_info {
    std::string hostname;      ///< Server hostname
    std::string ip_address;    ///< Server IP address
    std::string os_type;       ///< Server operating system type
    std::string smb_version;   ///< SMB protocol version supported
    std::string nfs_version;   ///< NFS protocol version supported
    bool is_reachable = false; ///< True if server is reachable
};

/**
 * @brief Get information about a server
 * @param server Server hostname or IP address
 * @return Server information
 */
[[nodiscard]] FRAPPE_API result<server_info> get_server_info(std::string_view server) noexcept;

#ifdef _WIN32
/**
 * @brief List all mapped network drives (Windows only)
 * @return Vector of mapped drive information
 */
[[nodiscard]] FRAPPE_API result<std::vector<network_share_info>> list_mapped_drives() noexcept;
#endif

// ============================================================================
// 6.9 Removable Storage Devices
// ============================================================================

/**
 * @brief Enumeration of removable device types
 */
enum class removable_device_type {
    unknown,      ///< Unknown device type
    usb_drive,    ///< USB drive (generic)
    usb_stick,    ///< USB flash drive
    sd_card,      ///< SD/microSD card
    cf_card,      ///< CompactFlash card
    external_hdd, ///< External hard disk drive
    external_ssd, ///< External solid state drive
    optical,      ///< Optical drive
    floppy,       ///< Floppy disk drive
    mtp,          ///< Media Transfer Protocol device
    ptp           ///< Picture Transfer Protocol device
};

/**
 * @brief Get the string name of a removable device type
 * @param type The removable device type
 * @return String view of the device type name
 */
[[nodiscard]] FRAPPE_API std::string_view removable_device_type_name(removable_device_type type) noexcept;

struct removable_device_info {
    path device_path;
    path mount_point;
    removable_device_type device_type = removable_device_type::unknown;
    std::string vendor;
    std::string product;
    std::string serial;
    std::uintmax_t size = 0;
    filesystem_type fs_type = filesystem_type::unknown;
    std::string label;
    bool is_mounted = false;
    bool is_writable = true;
    bool is_ejectable = true;
    std::string bus_type;    // USB2/USB3/Thunderbolt
    std::string usb_version; // 2.0/3.0/3.1
};

/**
 * @brief List all removable devices
 * @return Vector of removable device information
 */
[[nodiscard]] FRAPPE_API result<std::vector<removable_device_info>> list_removable_devices() noexcept;

/**
 * @brief List all USB devices
 * @return Vector of USB device information
 */
[[nodiscard]] FRAPPE_API result<std::vector<removable_device_info>> list_usb_devices() noexcept;

/**
 * @brief List all optical drives
 * @return Vector of optical drive information
 */
[[nodiscard]] FRAPPE_API result<std::vector<removable_device_info>> list_optical_drives() noexcept;

/**
 * @brief List all card readers
 * @return Vector of card reader information
 */
[[nodiscard]] FRAPPE_API result<std::vector<removable_device_info>> list_card_readers() noexcept;

/**
 * @brief Get removable device information for a path
 * @param p Path on the removable device
 * @return Removable device information
 */
[[nodiscard]] FRAPPE_API result<removable_device_info> get_removable_device_info(const path &p) noexcept;

/**
 * @brief Check if a device is ready
 * @param p Path on the device
 * @return True if the device is ready
 */
[[nodiscard]] FRAPPE_API result<bool> is_device_ready(const path &p) noexcept;

/**
 * @brief Check if media is present in a device
 * @param p Path on the device
 * @return True if media is present
 */
[[nodiscard]] FRAPPE_API result<bool> is_media_present(const path &p) noexcept;

/**
 * @brief Enumeration of device status values
 */
enum class device_status {
    unknown,   ///< Unknown status
    ready,     ///< Device is ready
    not_ready, ///< Device is not ready
    no_media,  ///< No media present
    error,     ///< Device error
    ejecting,  ///< Device is ejecting
    busy       ///< Device is busy
};

/**
 * @brief Get the string name of a device status
 * @param status The device status
 * @return String view of the status name
 */
[[nodiscard]] FRAPPE_API std::string_view device_status_name(device_status status) noexcept;

/**
 * @brief Get the status of a device
 * @param p Path on the device
 * @return Device status
 */
[[nodiscard]] FRAPPE_API result<device_status> get_device_status(const path &p) noexcept;

// ============================================================================
// 6.10 Cloud Storage Mounts
// ============================================================================

/**
 * @brief Enumeration of cloud storage providers
 */
enum class cloud_provider {
    unknown,      ///< Unknown provider
    onedrive,     ///< Microsoft OneDrive
    google_drive, ///< Google Drive
    dropbox,      ///< Dropbox
    icloud,       ///< Apple iCloud
    box,          ///< Box
    amazon_drive, ///< Amazon Drive
    nextcloud,    ///< Nextcloud
    owncloud      ///< ownCloud
};

/**
 * @brief Get the string name of a cloud provider
 * @param provider The cloud provider
 * @return String view of the provider name
 */
[[nodiscard]] FRAPPE_API std::string_view cloud_provider_name(cloud_provider provider) noexcept;

/**
 * @brief Structure containing cloud storage information
 */
struct cloud_storage_info {
    cloud_provider provider = cloud_provider::unknown; ///< Cloud provider
    std::string account;                               ///< Account name
    path mount_point;                                  ///< Local mount point
    std::string remote_root;                           ///< Remote root path
    bool is_online = false;                            ///< True if online
    bool is_syncing = false;                           ///< True if syncing
    std::uintmax_t quota_total = 0;                    ///< Total quota in bytes
    std::uintmax_t quota_used = 0;                     ///< Used quota in bytes
};

/**
 * @brief Check if a path is on cloud storage
 * @param p Path to check
 * @return True if on cloud storage
 */
[[nodiscard]] FRAPPE_API result<bool> is_cloud_path(const path &p) noexcept;

/**
 * @brief Get the cloud provider for a path
 * @param p Path to check
 * @return Cloud provider
 */
[[nodiscard]] FRAPPE_API result<cloud_provider> get_cloud_provider(const path &p) noexcept;

/**
 * @brief Get cloud storage information for a path
 * @param p Path on the cloud storage
 * @return Cloud storage information
 */
[[nodiscard]] FRAPPE_API result<cloud_storage_info> get_cloud_storage_info(const path &p) noexcept;

/**
 * @brief List all cloud storage mounts
 * @return Vector of cloud storage information
 */
[[nodiscard]] FRAPPE_API result<std::vector<cloud_storage_info>> list_cloud_mounts() noexcept;

// ============================================================================
// 6.11 Virtual File Systems
// ============================================================================

/**
 * @brief Enumeration of virtual filesystem types
 */
enum class virtual_fs_type {
    unknown,   ///< Unknown type
    fuse,      ///< FUSE (Filesystem in Userspace)
    winfsp,    ///< WinFsp (Windows File System Proxy)
    dokan,     ///< Dokan
    overlay,   ///< OverlayFS
    unionfs,   ///< UnionFS
    bindfs,    ///< BindFS
    sshfs,     ///< SSHFS (SSH Filesystem)
    encfs,     ///< EncFS (Encrypted Filesystem)
    veracrypt, ///< VeraCrypt
    loop,      ///< Loop device
    tmpfs,     ///< tmpfs (temporary filesystem)
    ramfs,     ///< ramfs (RAM filesystem)
    procfs,    ///< procfs (process filesystem)
    sysfs      ///< sysfs (system filesystem)
};

/**
 * @brief Get the string name of a virtual filesystem type
 * @param type The virtual filesystem type
 * @return String view of the type name
 */
[[nodiscard]] FRAPPE_API std::string_view virtual_fs_type_name(virtual_fs_type type) noexcept;

/**
 * @brief Structure containing virtual filesystem information
 */
struct virtual_fs_info {
    virtual_fs_type type = virtual_fs_type::unknown; ///< Virtual filesystem type
    path mount_point;                                ///< Mount point path
    std::string source;                              ///< Source specification
    std::vector<std::string> options;                ///< Mount options
    bool is_encrypted = false;                       ///< True if encrypted
    path backing_file;                               ///< Backing file path
};

/**
 * @brief Check if a path is on a virtual filesystem
 * @param p Path to check
 * @return True if on a virtual filesystem
 */
[[nodiscard]] FRAPPE_API result<bool> is_virtual_filesystem(const path &p) noexcept;

/**
 * @brief Check if a path is on a FUSE mount
 * @param p Path to check
 * @return True if on a FUSE mount
 */
[[nodiscard]] FRAPPE_API result<bool> is_fuse_mount(const path &p) noexcept;

/**
 * @brief Check if a path is on an encrypted mount
 * @param p Path to check
 * @return True if on an encrypted mount
 */
[[nodiscard]] FRAPPE_API result<bool> is_encrypted_mount(const path &p) noexcept;

/**
 * @brief Get virtual filesystem information for a path
 * @param p Path on the virtual filesystem
 * @return Virtual filesystem information
 */
[[nodiscard]] FRAPPE_API result<virtual_fs_info> get_virtual_fs_info(const path &p) noexcept;

/**
 * @brief List all virtual filesystem mounts
 * @return Vector of virtual filesystem information
 */
[[nodiscard]] FRAPPE_API result<std::vector<virtual_fs_info>> list_virtual_mounts() noexcept;

// ============================================================================
// 6.12 Mount Point Management
// ============================================================================

/**
 * @brief Structure containing mount point information
 */
struct mount_point_info {
    path mount_path;                                                     ///< Mount point path
    std::string device;                                                  ///< Device name
    filesystem_type fs_type = filesystem_type::unknown;                  ///< Filesystem type
    storage_location_type storage_type = storage_location_type::unknown; ///< Storage location type
    std::vector<std::string> options;                                    ///< Mount options
    bool is_readonly = false;                                            ///< True if read-only
    bool is_noexec = false;                                              ///< True if noexec
    bool is_nosuid = false;                                              ///< True if nosuid
    bool is_nodev = false;                                               ///< True if nodev
    bool is_bind = false;                                                ///< True if bind mount
    bool is_loop = false;                                                ///< True if loop mount
    bool is_automount = false;                                           ///< True if automounted
    std::filesystem::file_time_type mount_time;                          ///< Mount time
    path parent_mount;                                                   ///< Parent mount point
};

/**
 * @brief List all mount points
 * @return Vector of mount point information
 */
[[nodiscard]] FRAPPE_API result<std::vector<mount_point_info>> list_all_mounts() noexcept;

/**
 * @brief List all local mount points
 * @return Vector of local mount point information
 */
[[nodiscard]] FRAPPE_API result<std::vector<mount_point_info>> list_local_mounts() noexcept;

/**
 * @brief List all network mount points
 * @return Vector of network mount point information
 */
[[nodiscard]] FRAPPE_API result<std::vector<mount_point_info>> list_network_mounts() noexcept;

/**
 * @brief List all removable mount points
 * @return Vector of removable mount point information
 */
[[nodiscard]] FRAPPE_API result<std::vector<mount_point_info>> list_removable_mounts() noexcept;

/**
 * @brief Get mount point information for a path
 * @param p Path to query
 * @return Mount point information
 */
[[nodiscard]] FRAPPE_API result<mount_point_info> get_mount_point_info(const path &p) noexcept;

/**
 * @brief Find the mount point containing a path
 * @param p Path to query
 * @return Mount point path
 */
[[nodiscard]] FRAPPE_API result<path> find_mount_point(const path &p) noexcept;

/**
 * @brief Get all mount points under a path
 * @param p Path to query
 * @return Vector of mount point paths
 */
[[nodiscard]] FRAPPE_API result<std::vector<path>> get_all_mount_points_under(const path &p) noexcept;

/**
 * @brief Check if a path is a mount point
 * @param p Path to check
 * @return True if the path is a mount point
 */
[[nodiscard]] FRAPPE_API result<bool> is_mount_point(const path &p) noexcept;

/**
 * @brief Get the mount source for a path
 * @param p Path to query
 * @return Mount source string
 */
[[nodiscard]] FRAPPE_API result<std::string> get_mount_source(const path &p) noexcept;

/**
 * @brief Structure representing a node in the mount hierarchy tree
 */
struct mount_tree_node {
    mount_point_info info;                 ///< Mount point information
    std::vector<mount_tree_node> children; ///< Child mount points
};

/**
 * @brief Get the mount hierarchy tree
 * @return Root node of the mount tree
 */
[[nodiscard]] FRAPPE_API result<mount_tree_node> get_mount_tree() noexcept;

/**
 * @brief Get the parent mount point for a path
 * @param p Path to query
 * @return Parent mount point path
 */
[[nodiscard]] FRAPPE_API result<path> get_parent_mount(const path &p) noexcept;

/**
 * @brief Get child mount points for a path
 * @param p Path to query
 * @return Vector of child mount point paths
 */
[[nodiscard]] FRAPPE_API result<std::vector<path>> get_child_mounts(const path &p) noexcept;

/**
 * @brief Check if a path is a submount
 * @param p Path to check
 * @return True if the path is a submount
 */
[[nodiscard]] FRAPPE_API result<bool> is_submount(const path &p) noexcept;

} // namespace frappe

#endif // FRAPPE_FILESYSTEM_HPP
