#include <iostream>
#include <vector>
#include "frappe/frappe.hpp"

int main() {
    std::cout << "=== PCRE2 Regex Examples ===" << std::endl;
    
    auto re = frappe::compile_regex(R"(file(\d+)\.txt)");
    
    std::cout << "\nPattern: " << re.pattern() << std::endl;
    
    std::vector<std::string> test_strings = {
        "file123.txt",
        "file1.txt",
        "file.txt",
        "fileabc.txt",
        "FILE123.TXT"
    };
    
    std::cout << "\n--- Match tests ---" << std::endl;
    for (const auto& s : test_strings) {
        std::cout << "  match('" << s << "') = " 
                  << (re.match(s) ? "true" : "false") << std::endl;
    }
    
    std::cout << "\n--- Case-insensitive match ---" << std::endl;
    auto re_ci = frappe::compile_regex(R"(file(\d+)\.txt)", frappe::regex_options::caseless);
    for (const auto& s : test_strings) {
        std::cout << "  match('" << s << "') = " 
                  << (re_ci.match(s) ? "true" : "false") << std::endl;
    }
    
    std::cout << "\n--- Search tests ---" << std::endl;
    auto re_search = frappe::compile_regex(R"(\d+)");
    std::vector<std::string> search_strings = {
        "hello world",
        "file123",
        "no numbers here",
        "test456test"
    };
    for (const auto& s : search_strings) {
        std::cout << "  search('" << s << "') = " 
                  << (re_search.search(s) ? "true" : "false") << std::endl;
    }
    
    std::cout << "\n--- Replace tests ---" << std::endl;
    auto re_replace = frappe::compile_regex(R"(_)");
    std::string input = "hello_world_test";
    auto result = re_replace.replace(input, "-");
    if (result) {
        std::cout << "  '" << input << "' -> '" << *result << "'" << std::endl;
    }
    
    std::cout << "\n--- Path filtering with ranges ---" << std::endl;
    std::vector<frappe::path> paths = {
        "/src/main.cpp",
        "/src/util.hpp",
        "/src/test.cpp",
        "/include/header.hpp",
        "/docs/readme.txt"
    };
    
    std::cout << "All paths:" << std::endl;
    for (const auto& p : paths) {
        std::cout << "  " << p << std::endl;
    }
    
    std::cout << "\nFiltered (*.cpp):" << std::endl;
    for (const auto& p : paths | frappe::filter_regex(R"(.*\.cpp)")) {
        std::cout << "  " << p << std::endl;
    }
    
    std::cout << "\nFiltered NOT (*.cpp):" << std::endl;
    for (const auto& p : paths | frappe::filter_regex_not(R"(.*\.cpp)")) {
        std::cout << "  " << p << std::endl;
    }
    
    return 0;
}
