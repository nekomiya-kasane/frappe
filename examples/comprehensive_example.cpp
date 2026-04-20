#include <frappe/async.hpp>
#include <frappe/disk.hpp>
#include <frappe/frappe.hpp>
#include <iomanip>
#include <iostream>

void print_section(const std::string &title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << " " << title << "\n";
    std::cout << std::string(60, '=') << "\n";
}

void demo_path_operations() {
    print_section("Path Operations");

    auto home = frappe::home_path();
    if (home) std::cout << "Home path: " << home->string() << "\n";

    auto exe = frappe::executable_path();
    if (exe) std::cout << "Executable: " << exe->string() << "\n";

    std::cout << "Temp path: " << std::filesystem::temp_directory_path().string() << "\n";

    auto docs = frappe::documents_path();
    if (docs) std::cout << "Documents: " << docs->string() << "\n";

    auto downloads = frappe::downloads_path();
    if (downloads) std::cout << "Downloads: " << downloads->string() << "\n";
}

void demo_permissions() {
    print_section("Permissions");

    auto current_user = frappe::current_user_name();
    std::cout << "Current user: " << current_user << "\n";

    auto temp = std::filesystem::temp_directory_path();
    auto perms = frappe::get_permissions(temp);
    if (perms) {
        std::cout << "Temp permissions: " << frappe::permissions_to_string(*perms) << "\n";
    }

#ifdef _WIN32
    auto sid = frappe::get_sid(current_user);
    if (sid) {
        std::cout << "User SID: " << sid->sid_string << "\n";
        std::cout << "User RID: " << sid->rid << "\n";
        std::cout << "Domain: " << sid->domain << "\n";
    }

    std::cout << "Computer name: " << frappe::get_computer_name() << "\n";
    std::cout << "Domain joined: " << (frappe::is_domain_joined() ? "Yes" : "No") << "\n";
#endif
}

void demo_disk_info() {
    print_section("Disk Information");

    auto disks = frappe::list_disks();
    if (disks) {
        std::cout << "Found " << disks->size() << " disk(s):\n\n";
        for (const auto &disk : *disks) {
            std::cout << "  Device: " << disk.device_path << "\n";
            std::cout << "  Model: " << disk.model << "\n";
            std::cout << "  Size: " << (disk.size / (1024 * 1024 * 1024)) << " GB\n";
            std::cout << "  Type: " << frappe::disk_type_name(disk.type) << "\n";
            std::cout << "  Bus: " << frappe::disk_bus_type_name(disk.bus_type) << "\n";
            std::cout << "  Partitions: " << disk.partition_count << "\n";
            std::cout << "\n";
        }
    }
}

void demo_partitions() {
    print_section("Partition Information");

    auto partitions = frappe::list_mounted_partitions();
    if (partitions) {
        std::cout << "Mounted partitions:\n\n";
        for (const auto &part : *partitions) {
            std::cout << "  " << part.mount_point << "\n";
            std::cout << "    Size: " << (part.size / (1024 * 1024 * 1024)) << " GB\n";
            std::cout << "    Type: " << frappe::partition_type_name(part.type) << "\n";
            std::cout << "\n";
        }
    }
}

void demo_mounts() {
    print_section("Mount Information");

    auto mounts = frappe::list_mounts();
    if (mounts) {
        std::cout << "Active mounts:\n\n";
        for (const auto &m : *mounts) {
            std::cout << "  " << m.mount_point << "\n";
            std::cout << "    Device: " << m.device << "\n";
            std::cout << "    FS Type: " << m.fs_type_name << "\n";
            std::cout << "    Readonly: " << (m.is_readonly ? "Yes" : "No") << "\n";
            std::cout << "    Network: " << (m.is_network ? "Yes" : "No") << "\n";
            std::cout << "\n";
        }
    }
}

void demo_filesystem() {
    print_section("Filesystem Features");

    auto temp = std::filesystem::temp_directory_path();

    auto fs_type = frappe::get_filesystem_type(temp);
    if (fs_type) {
        std::cout << "Temp filesystem: " << frappe::filesystem_type_name(*fs_type) << "\n";
    }

    auto features = frappe::get_filesystem_features(temp);
    if (features) {
        std::cout << "Features:\n";
        std::cout << "  Case sensitive: " << (features->case_sensitive ? "Yes" : "No") << "\n";
        std::cout << "  Symlinks: " << (features->supports_symlinks ? "Yes" : "No") << "\n";
        std::cout << "  Hard links: " << (features->supports_hard_links ? "Yes" : "No") << "\n";
        std::cout << "  ACL: " << (features->supports_acl ? "Yes" : "No") << "\n";
        std::cout << "  Max filename: " << features->max_filename_length << "\n";
    }

    auto volumes = frappe::list_volumes();
    if (volumes) {
        std::cout << "\nVolumes: " << volumes->size() << "\n";
    }
}

void demo_async() {
    print_section("Async Operations");

    std::cout << "Async disk listing...\n";
    auto sender = frappe::list_disks_sender();
    auto result = frappe::sync_wait(std::move(sender));

    if (result) {
        auto &[disks] = *result;
        if (disks) {
            std::cout << "Async found " << disks->size() << " disk(s)\n";
        }
    }

    std::cout << "Async on thread pool...\n";
    auto pool_sender = frappe::on_thread_pool(frappe::list_mounts_sender());
    auto pool_result = frappe::sync_wait(std::move(pool_sender));

    if (pool_result) {
        auto &[mounts] = *pool_result;
        if (mounts) {
            std::cout << "Thread pool found " << mounts->size() << " mount(s)\n";
        }
    }
}

int main() {
    std::cout << "frappe Comprehensive Example\n";
    std::cout << "Version: " << frappe::version::string() << "\n";

    demo_path_operations();
    demo_permissions();
    demo_disk_info();
    demo_partitions();
    demo_mounts();
    demo_filesystem();
    demo_async();

    print_section("Done");
    std::cout << "All demonstrations completed successfully!\n";

    return 0;
}
