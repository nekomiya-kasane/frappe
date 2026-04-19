// Entry Pipeline Example: Demonstrates file_entry with pipeline operators and formatters
//
// This example shows how to use:
// - file_entry structure for aggregated file attributes
// - Projection functions for attribute access
// - Filter adapters for various filtering criteria
// - Sort adapters for flexible sorting
// - Aggregator adapters for statistics and selection
// - Format utilities for flexible output formatting

#include <frappe/frappe.hpp>
#include <iostream>
#include <iomanip>

using namespace frappe;
using namespace frappe::size_literals;

void demo_basic_listing() {
    std::cout << "\n=== Basic Directory Listing ===\n\n";
    
    auto dir = std::filesystem::current_path();
    std::cout << "Listing: " << dir.string() << "\n\n";
    
    // List entries
    auto entries = list_entries(dir, {.include_hidden = false});
    
    std::cout << std::left << std::setw(40) << "Name" 
              << std::right << std::setw(10) << "Size" 
              << std::setw(20) << "Modified" << "\n";
    std::cout << std::string(70, '-') << "\n";
    
    for (const auto& e : entries | entry::sort_by_name | entry::take(10)) {
        std::cout << std::left << std::setw(40) << e.name
                  << std::right << std::setw(10) << format_size_human(e.size)
                  << std::setw(20) << format_time_ls(e.mtime) << "\n";
    }
}

void demo_filtering() {
    std::cout << "\n=== Filtering Examples ===\n\n";
    
    auto dir = std::filesystem::current_path();
    auto entries = list_entries_recursive(dir, {.include_hidden = false, .max_depth = 3});
    
    // Filter by size
    std::cout << "Files larger than 10KB:\n";
    for (const auto& e : entries | files_only | size_gt(10_KiB) | entry::sort_by_size_desc | entry::take(5)) {
        std::cout << "  " << std::setw(8) << format_size_human(e.size) << "  " << e.name << "\n";
    }
    
    // Filter by extension
    std::cout << "\nC++ source files:\n";
    for (const auto& e : entries | files_only | extension_in({".cpp", ".hpp", ".h"}) | entry::take(5)) {
        std::cout << "  " << e.name << "\n";
    }
    
    // Filter by time
    std::cout << "\nFiles modified today:\n";
    for (const auto& e : entries | files_only | modified_today() | entry::take(5)) {
        std::cout << "  " << format_time(e.mtime, "%H:%M") << "  " << e.name << "\n";
    }
}

void demo_sorting() {
    std::cout << "\n=== Sorting Examples ===\n\n";
    
    auto dir = std::filesystem::current_path();
    auto entries = list_entries(dir);
    
    // Sort by size (largest first)
    std::cout << "Largest files:\n";
    for (const auto& e : entries | files_only | entry::sort_by_size_desc | entry::take(5)) {
        std::cout << "  " << std::setw(8) << format_size_human(e.size) << "  " << e.name << "\n";
    }
    
    // Sort by modification time (newest first)
    std::cout << "\nMost recently modified:\n";
    for (const auto& e : entries | entry::sort_by_mtime_desc | entry::take(5)) {
        std::cout << "  " << format_time_ls(e.mtime) << "  " << e.name << "\n";
    }
    
    // Sort by extension
    std::cout << "\nSorted by extension:\n";
    for (const auto& e : entries | files_only | entry::sort_by_extension | entry::take(5)) {
        std::cout << "  " << e.name << "\n";
    }
}

void demo_aggregation() {
    std::cout << "\n=== Aggregation Examples ===\n\n";
    
    auto dir = std::filesystem::current_path();
    auto entries = list_entries_recursive(dir, {.include_hidden = false});
    
    // Count files
    auto file_count = entries | files_only | count;
    auto dir_count = entries | dirs_only | count;
    std::cout << "Files: " << file_count << ", Directories: " << dir_count << "\n";
    
    // Total size
    auto total = entries | files_only | entry::total_size;
    std::cout << "Total size: " << format_size_human(total) << "\n";
    
    // Find largest file
    auto largest_file = entries | files_only | largest;
    if (largest_file) {
        std::cout << "Largest file: " << largest_file->name 
                  << " (" << format_size_human(largest_file->size) << ")\n";
    }
    
    // Find newest file
    auto newest_file = entries | files_only | newest;
    if (newest_file) {
        std::cout << "Newest file: " << newest_file->name 
                  << " (" << format_time_ls(newest_file->mtime) << ")\n";
    }
    
    // Group by extension and count
    std::cout << "\nFiles by extension:\n";
    auto by_ext = entries | files_only | count_by(proj::extension);
    for (const auto& [ext, cnt] : by_ext) {
        if (cnt >= 2) {
            std::cout << "  " << (ext.empty() ? "(no ext)" : ext) << ": " << cnt << "\n";
        }
    }
}

void demo_formatting() {
    std::cout << "\n=== Formatting Examples ===\n\n";
    
    auto dir = std::filesystem::current_path();
    auto entries = list_entries(dir) | entry::take(5);
    
    // Different format styles
    std::cout << "Name only:\n";
    for (const auto& e : entries) {
        std::cout << "  " << e.name << "\n";
    }
    
    std::cout << "\nWith size (human readable):\n";
    for (const auto& e : entries) {
        std::cout << "  " << e.name << " - " << format_size_human(e.size) << "\n";
    }
    
    std::cout << "\nls -l style:\n";
    for (const auto& e : entries) {
        std::cout << format_file_type_char(e.type) << format_permissions(e.permissions)
                  << " " << std::setw(3) << e.hard_link_count
                  << " " << std::setw(8) << e.owner
                  << " " << std::setw(8) << e.group
                  << " " << std::setw(8) << e.size
                  << " " << format_time_ls(e.mtime)
                  << " " << e.name;
        if (e.is_symlink) {
            std::cout << " -> " << e.symlink_target.string();
        }
        std::cout << "\n";
    }
    
    std::cout << "\nWith inode:\n";
    for (const auto& e : entries) {
        std::cout << std::setw(10) << e.inode << " " << e.name << "\n";
    }
    
    // Custom time format
    std::cout << "\nCustom time format:\n";
    for (const auto& e : entries) {
        std::cout << "  " << e.name << " - " << format_time(e.mtime, "%Y/%m/%d") << "\n";
    }
}

void demo_complex_pipeline() {
    std::cout << "\n=== Complex Pipeline Example ===\n\n";
    
    auto dir = std::filesystem::current_path();
    
    // Find the 5 largest C++ files modified in the last week
    auto results = list_entries_recursive(dir, {.include_hidden = false})
        | files_only
        | extension_in({".cpp", ".hpp", ".h", ".c"})
        | modified_this_week()
        | entry::sort_by_size_desc
        | entry::take(5);
    
    std::cout << "Top 5 largest C++ files modified this week:\n";
    std::cout << std::left << std::setw(50) << "Path" 
              << std::right << std::setw(10) << "Size" 
              << std::setw(20) << "Modified" << "\n";
    std::cout << std::string(80, '-') << "\n";
    
    for (const auto& e : results) {
        std::cout << std::left << std::setw(50) << e.file_path.filename().string()
                  << std::right << std::setw(10) << format_size_human(e.size)
                  << std::setw(20) << format_time_ls(e.mtime) << "\n";
    }
    
    // Calculate total size
    auto total = results | entry::total_size;
    std::cout << "\nTotal: " << format_size_human(total) << "\n";
}

int main() {
    std::cout << "frappe Entry Pipeline and Formatter Example\n";
    std::cout << "============================================\n";
    
    try {
        demo_basic_listing();
        demo_filtering();
        demo_sorting();
        demo_aggregation();
        demo_formatting();
        demo_complex_pipeline();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "\n=== Done ===\n";
    return 0;
}
