#include "frappe/filesystem.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

class FilesystemTest : public ::testing::Test {
  protected:
    void SetUp() override {
        _test_dir = std::filesystem::temp_directory_path() / "frappe_filesystem_test";
        std::filesystem::create_directories(_test_dir);

        _test_file = _test_dir / "test_file.txt";
        std::ofstream(_test_file) << "test content";
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(_test_dir, ec);
    }

    std::filesystem::path _test_dir;
    std::filesystem::path _test_file;
};

// ============================================================================
// 6.1 Disk Space Tests
// ============================================================================

TEST_F(FilesystemTest, Space) {
    const auto info = frappe::space(_test_dir);
    EXPECT_TRUE(info.has_value());
    EXPECT_GT(info->capacity, 0u);
    EXPECT_GT(info->free, 0u);
    EXPECT_LE(info->available, info->free);
}

TEST_F(FilesystemTest, SpaceNonExistent) {
    const auto info = frappe::space("/nonexistent/path/xyz");
    EXPECT_FALSE(info.has_value());
}

// ============================================================================
// 6.2 Filesystem Type Tests
// ============================================================================

TEST_F(FilesystemTest, GetFilesystemType) {
    const auto type = frappe::get_filesystem_type(_test_dir);
    EXPECT_TRUE(type.has_value());
    EXPECT_NE(*type, frappe::filesystem_type::unknown);
}

TEST_F(FilesystemTest, FilesystemTypeName) {
    EXPECT_EQ(frappe::filesystem_type_name(frappe::filesystem_type::ntfs), "ntfs");
    EXPECT_EQ(frappe::filesystem_type_name(frappe::filesystem_type::ext4), "ext4");
    EXPECT_EQ(frappe::filesystem_type_name(frappe::filesystem_type::apfs), "apfs");
    EXPECT_EQ(frappe::filesystem_type_name(frappe::filesystem_type::unknown), "unknown");
}

// ============================================================================
// 6.3 Filesystem Features Tests
// ============================================================================

TEST_F(FilesystemTest, GetFilesystemFeatures) {
    const auto features = frappe::get_filesystem_features(_test_dir);
    EXPECT_TRUE(features.has_value());
    EXPECT_GT(features->max_filename_length, 0u);
}

TEST_F(FilesystemTest, FilesystemFeaturesType) {
    const auto features = frappe::get_filesystem_features(_test_dir);
    EXPECT_TRUE(features.has_value());

    const auto type = frappe::get_filesystem_type(_test_dir);
    EXPECT_TRUE(type.has_value());

    EXPECT_EQ(features->type, *type);
}

// ============================================================================
// 6.5 Volume Information Tests
// ============================================================================

TEST_F(FilesystemTest, GetVolumeInfo) {
    const auto info = frappe::get_volume_info(_test_dir);
    EXPECT_TRUE(info.has_value());
    EXPECT_FALSE(info->mount_point.empty());
    EXPECT_GT(info->total_size, 0u);
}

TEST_F(FilesystemTest, ListVolumes) {
    const auto volumes = frappe::list_volumes();
    EXPECT_TRUE(volumes.has_value());
    EXPECT_GT(volumes->size(), 0u);

    for (const auto &vol : *volumes) {
        EXPECT_FALSE(vol.mount_point.empty());
    }
}

// ============================================================================
// 6.6 Storage Location Type Tests
// ============================================================================

TEST_F(FilesystemTest, GetStorageLocationType) {
    const auto type = frappe::get_storage_location_type(_test_dir);
    EXPECT_TRUE(type.has_value());
}

TEST_F(FilesystemTest, StorageLocationTypeName) {
    EXPECT_EQ(frappe::storage_location_type_name(frappe::storage_location_type::local), "local");
    EXPECT_EQ(frappe::storage_location_type_name(frappe::storage_location_type::removable), "removable");
    EXPECT_EQ(frappe::storage_location_type_name(frappe::storage_location_type::network_share), "network_share");
}

TEST_F(FilesystemTest, IsLocalStorage) {
    const auto is_local = frappe::is_local_storage(_test_dir);
    EXPECT_TRUE(is_local.has_value());
    // Temp directory should typically be on local storage
}

TEST_F(FilesystemTest, IsNetworkStorage) {
    const auto is_network = frappe::is_network_storage(_test_dir);
    EXPECT_TRUE(is_network.has_value());
    // Temp directory should not be on network storage
    EXPECT_FALSE(*is_network);
}

TEST_F(FilesystemTest, IsVirtualStorage) {
    const auto is_virtual = frappe::is_virtual_storage(_test_dir);
    EXPECT_TRUE(is_virtual.has_value());
}

// ============================================================================
// 6.8 Network Filesystem Tests
// ============================================================================

TEST_F(FilesystemTest, NetworkProtocolName) {
    EXPECT_EQ(frappe::network_protocol_name(frappe::network_protocol::smb), "smb");
    EXPECT_EQ(frappe::network_protocol_name(frappe::network_protocol::nfs), "nfs");
    EXPECT_EQ(frappe::network_protocol_name(frappe::network_protocol::webdav), "webdav");
}

TEST_F(FilesystemTest, IsNetworkPath) {
    const auto is_network = frappe::is_network_path(_test_dir);
    EXPECT_TRUE(is_network.has_value());
    EXPECT_FALSE(*is_network);
}

TEST_F(FilesystemTest, IsRemoteFilesystem) {
    const auto is_remote = frappe::is_remote_filesystem(_test_dir);
    EXPECT_TRUE(is_remote.has_value());
    EXPECT_FALSE(*is_remote);
}

#ifdef _WIN32
TEST_F(FilesystemTest, IsUncPath) {
    auto is_unc = frappe::is_unc_path("\\\\server\\share");
    EXPECT_TRUE(is_unc.has_value());
    EXPECT_TRUE(*is_unc);

    is_unc = frappe::is_unc_path("C:\\Users");
    EXPECT_TRUE(is_unc.has_value());
    EXPECT_FALSE(*is_unc);
}

TEST_F(FilesystemTest, ParseUncPath) {
    const auto components = frappe::parse_unc_path("\\\\server\\share\\folder\\file.txt");
    EXPECT_TRUE(components.has_value());
    EXPECT_EQ(components->server, "server");
    EXPECT_EQ(components->share, "share");
    EXPECT_EQ(components->path, "\\folder\\file.txt");
}

TEST_F(FilesystemTest, ParseUncPathServerOnly) {
    const auto components = frappe::parse_unc_path("\\\\server");
    EXPECT_TRUE(components.has_value());
    EXPECT_EQ(components->server, "server");
    EXPECT_TRUE(components->share.empty());
}

TEST_F(FilesystemTest, ListMappedDrives) {
    const auto drives = frappe::list_mapped_drives();
    EXPECT_TRUE(drives.has_value());
    // May be empty if no network drives are mapped
}
#endif

TEST_F(FilesystemTest, ListNetworkShares) {
    const auto shares = frappe::list_network_shares();
    EXPECT_TRUE(shares.has_value());
    // May be empty if no network shares are mounted
}

// 6.8 Additional network functions
TEST_F(FilesystemTest, ListSmbShares) {
    const auto shares = frappe::list_smb_shares();
    EXPECT_TRUE(shares.has_value());
    // May be empty if no SMB shares are mounted
}

TEST_F(FilesystemTest, ListNfsMounts) {
    const auto mounts = frappe::list_nfs_mounts();
    EXPECT_TRUE(mounts.has_value());
    // May be empty if no NFS mounts
}

TEST_F(FilesystemTest, IsShareAvailable) {
    const auto available = frappe::is_share_available(_test_dir);
    EXPECT_TRUE(available.has_value());
    EXPECT_TRUE(*available); // Test dir should exist
}

TEST_F(FilesystemTest, GetServerInfo) {
    const auto info = frappe::get_server_info("localhost");
    EXPECT_TRUE(info.has_value());
    EXPECT_EQ(info->hostname, "localhost");
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(FilesystemTest, NonExistentPath) {
    // On Windows, paths with drive letters may still resolve to the drive's filesystem
    // Test with a truly invalid path
#ifdef _WIN32
    auto type = frappe::get_filesystem_type("Z:\\nonexistent\\path\\xyz");
    // May or may not fail depending on whether Z: exists
#else
    auto type = frappe::get_filesystem_type("/nonexistent/path/xyz");
    EXPECT_FALSE(type.has_value());
#endif
}

TEST_F(FilesystemTest, RootPath) {
#ifdef _WIN32
    const auto type = frappe::get_filesystem_type("C:\\");
#else
    auto type = frappe::get_filesystem_type("/");
#endif
    EXPECT_TRUE(type.has_value());
}

// ============================================================================
// 6.9 Removable Storage Device Tests
// ============================================================================

TEST_F(FilesystemTest, RemovableDeviceTypeName) {
    EXPECT_EQ(frappe::removable_device_type_name(frappe::removable_device_type::unknown), "unknown");
    EXPECT_EQ(frappe::removable_device_type_name(frappe::removable_device_type::usb_drive), "usb_drive");
    EXPECT_EQ(frappe::removable_device_type_name(frappe::removable_device_type::optical), "optical");
    EXPECT_EQ(frappe::removable_device_type_name(frappe::removable_device_type::sd_card), "sd_card");
}

TEST_F(FilesystemTest, ListRemovableDevices) {
    const auto devices = frappe::list_removable_devices();
    EXPECT_TRUE(devices.has_value());
    // May or may not have removable devices
}

TEST_F(FilesystemTest, ListUsbDevices) {
    const auto devices = frappe::list_usb_devices();
    EXPECT_TRUE(devices.has_value());
    // May be empty if no USB devices
}

TEST_F(FilesystemTest, ListOpticalDrives) {
    const auto drives = frappe::list_optical_drives();
    EXPECT_TRUE(drives.has_value());
    // May be empty if no optical drives
}

TEST_F(FilesystemTest, ListCardReaders) {
    const auto readers = frappe::list_card_readers();
    EXPECT_TRUE(readers.has_value());
    // May be empty if no card readers
}

TEST_F(FilesystemTest, IsDeviceReady) {
    const auto ready = frappe::is_device_ready(_test_dir);
    EXPECT_TRUE(ready.has_value());
    EXPECT_TRUE(*ready);
}

TEST_F(FilesystemTest, DeviceStatusName) {
    EXPECT_EQ(frappe::device_status_name(frappe::device_status::unknown), "unknown");
    EXPECT_EQ(frappe::device_status_name(frappe::device_status::ready), "ready");
    EXPECT_EQ(frappe::device_status_name(frappe::device_status::not_ready), "not_ready");
    EXPECT_EQ(frappe::device_status_name(frappe::device_status::no_media), "no_media");
    EXPECT_EQ(frappe::device_status_name(frappe::device_status::error), "error");
}

TEST_F(FilesystemTest, GetDeviceStatus) {
    const auto status = frappe::get_device_status(_test_dir);
    EXPECT_TRUE(status.has_value());
    EXPECT_EQ(*status, frappe::device_status::ready);
}

// ============================================================================
// 6.10 Cloud Storage Tests
// ============================================================================

TEST_F(FilesystemTest, CloudProviderName) {
    EXPECT_EQ(frappe::cloud_provider_name(frappe::cloud_provider::unknown), "unknown");
    EXPECT_EQ(frappe::cloud_provider_name(frappe::cloud_provider::onedrive), "onedrive");
    EXPECT_EQ(frappe::cloud_provider_name(frappe::cloud_provider::dropbox), "dropbox");
    EXPECT_EQ(frappe::cloud_provider_name(frappe::cloud_provider::google_drive), "google_drive");
    EXPECT_EQ(frappe::cloud_provider_name(frappe::cloud_provider::icloud), "icloud");
}

TEST_F(FilesystemTest, IsCloudPath) {
    const auto is_cloud = frappe::is_cloud_path(_test_dir);
    EXPECT_TRUE(is_cloud.has_value());
    // Test path is not a cloud path
}

TEST_F(FilesystemTest, GetCloudProvider) {
    const auto provider = frappe::get_cloud_provider(_test_dir);
    EXPECT_TRUE(provider.has_value());
    // Test path is not a cloud path, should return unknown
}

TEST_F(FilesystemTest, ListCloudMounts) {
    const auto mounts = frappe::list_cloud_mounts();
    EXPECT_TRUE(mounts.has_value());
    // May or may not have cloud mounts
}

// ============================================================================
// 6.11 Virtual Filesystem Tests
// ============================================================================

TEST_F(FilesystemTest, VirtualFsTypeName) {
    EXPECT_EQ(frappe::virtual_fs_type_name(frappe::virtual_fs_type::unknown), "unknown");
    EXPECT_EQ(frappe::virtual_fs_type_name(frappe::virtual_fs_type::fuse), "fuse");
    EXPECT_EQ(frappe::virtual_fs_type_name(frappe::virtual_fs_type::tmpfs), "tmpfs");
    EXPECT_EQ(frappe::virtual_fs_type_name(frappe::virtual_fs_type::overlay), "overlay");
}

TEST_F(FilesystemTest, IsVirtualFilesystem) {
    const auto is_virtual = frappe::is_virtual_filesystem(_test_dir);
    EXPECT_TRUE(is_virtual.has_value());
}

TEST_F(FilesystemTest, IsFuseMount) {
    const auto is_fuse = frappe::is_fuse_mount(_test_dir);
    EXPECT_TRUE(is_fuse.has_value());
    EXPECT_FALSE(*is_fuse); // Test path should not be FUSE
}

TEST_F(FilesystemTest, IsEncryptedMount) {
    const auto is_encrypted = frappe::is_encrypted_mount(_test_dir);
    EXPECT_TRUE(is_encrypted.has_value());
}

TEST_F(FilesystemTest, ListVirtualMounts) {
    const auto mounts = frappe::list_virtual_mounts();
    EXPECT_TRUE(mounts.has_value());
}

// ============================================================================
// 6.12 Mount Point Management Tests
// ============================================================================

TEST_F(FilesystemTest, ListAllMounts) {
    const auto mounts = frappe::list_all_mounts();
    EXPECT_TRUE(mounts.has_value());
    EXPECT_GT(mounts->size(), 0u);

    // Every mount should have a path
    for (const auto &m : *mounts) {
        EXPECT_FALSE(m.mount_path.empty());
    }
}

TEST_F(FilesystemTest, ListLocalMounts) {
    const auto mounts = frappe::list_local_mounts();
    EXPECT_TRUE(mounts.has_value());

    for (const auto &m : *mounts) {
        EXPECT_EQ(m.storage_type, frappe::storage_location_type::local);
    }
}

TEST_F(FilesystemTest, ListNetworkMounts) {
    const auto mounts = frappe::list_network_mounts();
    EXPECT_TRUE(mounts.has_value());
    // May be empty if no network mounts
}

TEST_F(FilesystemTest, ListRemovableMounts) {
    const auto mounts = frappe::list_removable_mounts();
    EXPECT_TRUE(mounts.has_value());
    // May be empty if no removable devices
}

TEST_F(FilesystemTest, GetMountPointInfo) {
    const auto info = frappe::get_mount_point_info(_test_dir);
    EXPECT_TRUE(info.has_value());
    EXPECT_FALSE(info->mount_path.empty());
}

TEST_F(FilesystemTest, FindMountPoint) {
    const auto mp = frappe::find_mount_point(_test_dir);
    EXPECT_TRUE(mp.has_value());
    EXPECT_FALSE(mp->empty());
}

TEST_F(FilesystemTest, GetAllMountPointsUnder) {
#ifdef _WIN32
    const auto mounts = frappe::get_all_mount_points_under("C:\\");
#else
    auto mounts = frappe::get_all_mount_points_under("/");
#endif
    EXPECT_TRUE(mounts.has_value());
    // Root may have submounts
}

TEST_F(FilesystemTest, GetMountSource) {
    const auto source = frappe::get_mount_source(_test_dir);
    EXPECT_TRUE(source.has_value());
}

TEST_F(FilesystemTest, GetMountTree) {
    const auto tree = frappe::get_mount_tree();
    EXPECT_TRUE(tree.has_value());
}

TEST_F(FilesystemTest, GetParentMount) {
    // Create a subdirectory path
    const auto subdir = _test_dir / "subdir";
    std::filesystem::create_directories(subdir);

    auto parent = frappe::get_parent_mount(subdir);
    // May or may not have a parent mount depending on system

    std::filesystem::remove(subdir);
}

TEST_F(FilesystemTest, GetChildMounts) {
#ifdef _WIN32
    const auto children = frappe::get_child_mounts("C:\\");
#else
    auto children = frappe::get_child_mounts("/");
#endif
    EXPECT_TRUE(children.has_value());
}

TEST_F(FilesystemTest, IsSubmount) {
    const auto is_sub = frappe::is_submount(_test_dir);
    EXPECT_TRUE(is_sub.has_value());
}
