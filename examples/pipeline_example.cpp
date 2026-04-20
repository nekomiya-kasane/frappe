// Pipeline Example: Directory traversal with sorting, filtering, and selection
//
// This example demonstrates how to use frappe's Range adapters with pipe operators
// to traverse a directory, sort by modification time, filter by regex, and take the first match.

#include <chrono>
#include <frappe/frappe.hpp>
#include <iomanip>
#include <iostream>

// Helper function to format file time
std::string format_time(std::filesystem::file_time_type ftime) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    auto time = std::chrono::system_clock::to_time_t(sctp);
    std::tm tm = *std::localtime(&time);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void demo_basic_pipeline() {
    std::cout << "\n=== Basic Pipeline: Sort by mtime, filter by regex, take first ===\n\n";

    // Get current directory or use temp directory
    auto search_dir = std::filesystem::current_path();
    std::cout << "Searching in: " << search_dir << "\n\n";

    // Example 1: Find the most recently modified .cpp file
    // Pipeline: directory_iterator -> transform to path -> sort by mtime (desc) -> filter regex -> first
    auto result = std::filesystem::directory_iterator(search_dir) |
                  std::views::transform([](const auto &entry) { return entry.path(); }) |
                  frappe::sort_by_mtime_desc            // Sort by modification time (newest first)
                  | frappe::filter_regex(R"(.*\.cpp$)") // Filter: only .cpp files
                  | frappe::first;                      // Take the first one

    if (result) {
        std::cout << "Most recently modified .cpp file:\n";
        std::cout << "  Path: " << result->string() << "\n";
        std::error_code ec;
        auto mtime = std::filesystem::last_write_time(*result, ec);
        if (!ec) {
            std::cout << "  Modified: " << format_time(mtime) << "\n";
        }
        auto size = std::filesystem::file_size(*result, ec);
        if (!ec) {
            std::cout << "  Size: " << size << " bytes\n";
        }
    } else {
        std::cout << "No .cpp files found.\n";
    }
}

void demo_take_multiple() {
    std::cout << "\n=== Take Multiple: Get top 5 largest files ===\n\n";

    auto search_dir = std::filesystem::current_path();

    // Find the 5 largest files
    auto largest_files = std::filesystem::directory_iterator(search_dir) |
                         std::views::transform([](const auto &entry) { return entry.path(); }) |
                         std::views::filter([](const auto &p) {
                             std::error_code ec;
                             return std::filesystem::is_regular_file(p, ec);
                         }) |
                         frappe::sort_by_size // Sort by size (ascending)
                         | frappe::reverse    // Reverse to get descending
                         | frappe::take(5);   // Take top 5

    std::cout << "Top 5 largest files:\n";
    for (const auto &p : largest_files) {
        std::error_code ec;
        auto size = std::filesystem::file_size(p, ec);
        std::cout << "  " << std::setw(10) << size << " bytes  " << p.filename().string() << "\n";
    }
}

void demo_recursive_search() {
    std::cout << "\n=== Recursive Search with find_lazy ===\n\n";

    auto search_dir = std::filesystem::current_path();

    // Use find_lazy for recursive search with conditions
    auto options = frappe::find_options{}.is_file().extension("hpp").max_depth(3);

    // Pipeline: find_lazy -> sort by name -> take first 10
    auto header_files = frappe::find_lazy(search_dir, options) | frappe::sort_by_name | frappe::take(10);

    std::cout << "First 10 .hpp files (sorted by name):\n";
    for (const auto &p : header_files) {
        std::cout << "  " << p.filename().string() << "\n";
    }
}

void demo_complex_pipeline() {
    std::cout << "\n=== Complex Pipeline: Multiple filters and transformations ===\n\n";

    auto search_dir = std::filesystem::current_path();

    // Find source files modified in the last 7 days, sorted by modification time
    auto now = std::filesystem::file_time_type::clock::now();
    auto seven_days_ago = now - std::chrono::hours(24 * 7);

    auto recent_sources =
        std::filesystem::recursive_directory_iterator(search_dir,
                                                      std::filesystem::directory_options::skip_permission_denied) |
        std::views::transform([](const auto &entry) { return entry.path(); }) | std::views::filter([](const auto &p) {
            std::error_code ec;
            return std::filesystem::is_regular_file(p, ec);
        }) |
        frappe::filter_regex(R"(.*\.(cpp|hpp|c|h)$)") // Source files only
        | std::views::filter([seven_days_ago](const auto &p) {
              std::error_code ec;
              auto mtime = std::filesystem::last_write_time(p, ec);
              return !ec && mtime > seven_days_ago;
          }) |
        frappe::sort_by_mtime_desc | frappe::take(10);

    std::cout << "Recently modified source files (last 7 days):\n";
    for (const auto &p : recent_sources) {
        std::error_code ec;
        auto mtime = std::filesystem::last_write_time(p, ec);
        std::cout << "  " << format_time(mtime) << "  " << p.filename().string() << "\n";
    }
}

void demo_with_find_options() {
    std::cout << "\n=== Using find_options with Pipeline ===\n\n";

    auto search_dir = std::filesystem::current_path();

    // Build search options
    using namespace frappe::literals;
    auto options = frappe::find_options{}.is_file().size_gt(1_KB).extensions({"cpp", "hpp"}).max_depth(5);

    // Execute search and process with pipeline
    auto files = frappe::find(search_dir, options) | frappe::sort_by_mtime_desc | frappe::take(5);

    std::cout << "Top 5 most recently modified source files (>1KB):\n";
    for (const auto &p : files) {
        std::error_code ec;
        auto size = std::filesystem::file_size(p, ec);
        auto mtime = std::filesystem::last_write_time(p, ec);
        std::cout << "  " << format_time(mtime) << "  " << std::setw(8) << size << " bytes  " << p.filename().string()
                  << "\n";
    }

    // Calculate total size
    auto total = frappe::total_size(files);
    std::cout << "\nTotal size: " << total << " bytes\n";
}

int main() {
    std::cout << "frappe Pipeline Example\n";
    std::cout << "=======================\n";

    try {
        demo_basic_pipeline();
        demo_take_multiple();
        demo_recursive_search();
        demo_complex_pipeline();
        demo_with_find_options();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\n=== Done ===\n";
    return 0;
}
