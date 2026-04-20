#include "frappe/frappe.hpp"

#include <iostream>

int main(int argc, char *argv[]) {
    std::string pattern = argc > 1 ? argv[1] : "*.txt";
    std::string base_path = argc > 2 ? argv[2] : ".";

    std::cout << "Glob pattern: " << pattern << std::endl;
    std::cout << "Base path: " << base_path << std::endl;
    std::cout << "\n=== Results ===" << std::endl;

    auto results = frappe::glob(base_path, pattern);

    for (const auto &p : results) {
        std::cout << "  " << p << std::endl;
    }

    std::cout << "\nTotal: " << results.size() << " files" << std::endl;

    std::cout << "\n=== Using Iterator ===" << std::endl;

    int count = 0;
    for (const auto &p : frappe::iglob(base_path, pattern)) {
        std::cout << "  " << p << std::endl;
        if (++count >= 5) {
            std::cout << "  ... (showing first 5)" << std::endl;
            break;
        }
    }

    std::cout << "\n=== fnmatch examples ===" << std::endl;

    std::cout << "fnmatch('file.txt', '*.txt') = " << (frappe::fnmatch("file.txt", "*.txt") ? "true" : "false")
              << std::endl;
    std::cout << "fnmatch('file.txt', '*.log') = " << (frappe::fnmatch("file.txt", "*.log") ? "true" : "false")
              << std::endl;
    std::cout << "fnmatch('file123.txt', 'file[0-9]*.txt') = "
              << (frappe::fnmatch("file123.txt", "file[0-9]*.txt") ? "true" : "false") << std::endl;

    return 0;
}
