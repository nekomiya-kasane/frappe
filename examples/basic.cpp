#include <iostream>
#include "frappe/frappe.hpp"

int main() {
    std::cout << "frappe version: " << frappe::version::string() << std::endl;
    std::cout << "Version number: " << frappe::version::number() << std::endl;
    
    std::cout << "\n=== Special Paths ===" << std::endl;
    
    if (auto home = frappe::home_path()) {
        std::cout << "Home: " << *home << std::endl;
    }
    
    if (auto exe = frappe::executable_path()) {
        std::cout << "Executable: " << *exe << std::endl;
    }
    
    if (auto app_data = frappe::app_data_path()) {
        std::cout << "App Data: " << *app_data << std::endl;
    }
    
    if (auto desktop = frappe::desktop_path()) {
        std::cout << "Desktop: " << *desktop << std::endl;
    }
    
    if (auto docs = frappe::documents_path()) {
        std::cout << "Documents: " << *docs << std::endl;
    }
    
    if (auto downloads = frappe::downloads_path()) {
        std::cout << "Downloads: " << *downloads << std::endl;
    }
    
    std::cout << "\n=== Path Resolution ===" << std::endl;
    
    if (auto resolved = frappe::resolve_path("~/test")) {
        std::cout << "~/test -> " << *resolved << std::endl;
    }
    
    return 0;
}
