#include <rpp/rpp.hpp>
#include <frappe/entry.hpp>
#include <frappe/entry_adapters.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <map>

struct Person {
    std::string name;
    int age;
    std::string department;
    double salary;
};

struct Department {
    std::string name;
    std::string location;
};

int main() {
    std::cout << "=== rpp (Range++) Examples ===\n\n";
    
    // Sample data
    std::vector<Person> people = {
        {"Alice", 30, "Engineering", 80000},
        {"Bob", 25, "Engineering", 70000},
        {"Charlie", 35, "Sales", 60000},
        {"Diana", 28, "Engineering", 75000},
        {"Eve", 32, "Sales", 65000},
        {"Frank", 40, "HR", 55000},
        {"Grace", 27, "HR", 52000},
    };
    
    std::vector<Department> departments = {
        {"Engineering", "Building A"},
        {"Sales", "Building B"},
        {"HR", "Building C"},
        {"Marketing", "Building D"},
    };
    
    // =========================================================================
    // 1. Basic filtering
    // =========================================================================
    std::cout << "1. Filter: Engineers with salary > 70000\n";
    auto high_paid_engineers = people 
        | rpp::filter([](const Person& p) { 
            return p.department == "Engineering" && p.salary > 70000; 
          })
        | rpp::to_vector;
    
    for (const auto& p : high_paid_engineers) {
        std::cout << "   " << p.name << " - $" << p.salary << "\n";
    }
    
    // =========================================================================
    // 2. SQL-style filters
    // =========================================================================
    std::cout << "\n2. SQL-style: Age BETWEEN 25 AND 32\n";
    auto age_range = people 
        | rpp::filters::between(&Person::age, 25, 32)
        | rpp::to_vector;
    
    for (const auto& p : age_range) {
        std::cout << "   " << p.name << " (age " << p.age << ")\n";
    }
    
    // =========================================================================
    // 3. Sorting with comparators
    // =========================================================================
    std::cout << "\n3. Sort by salary (descending)\n";
    auto sorted_by_salary = people 
        | rpp::sort_by(&Person::salary).desc();
    
    for (const auto& p : sorted_by_salary) {
        std::cout << "   " << p.name << " - $" << p.salary << "\n";
    }
    
    // =========================================================================
    // 4. Using comparators with STL containers
    // =========================================================================
    std::cout << "\n4. std::set with custom comparator (by name)\n";
    std::set<Person, decltype(rpp::compare_by(&Person::name))> people_set(
        rpp::compare_by(&Person::name));
    
    for (const auto& p : people) {
        people_set.insert(p);
    }
    
    for (const auto& p : people_set) {
        std::cout << "   " << p.name << "\n";
    }
    
    // =========================================================================
    // 5. Multi-level sorting
    // =========================================================================
    std::cout << "\n5. Multi-sort: by department, then by salary desc\n";
    auto multi_sorted = people 
        | rpp::multi_sort(
            rpp::sort_by(&Person::department),
            rpp::sort_by(&Person::salary).desc()
          );
    
    for (const auto& p : multi_sorted) {
        std::cout << "   " << p.department << " - " << p.name << " ($" << p.salary << ")\n";
    }
    
    // =========================================================================
    // 6. Aggregations
    // =========================================================================
    std::cout << "\n6. Aggregations\n";
    
    auto total_salary = people | rpp::sum(&Person::salary);
    auto avg_salary = people | rpp::avg(&Person::salary);
    auto max_salary_person = people | rpp::max_by(&Person::salary);
    
    std::cout << "   Total salary: $" << total_salary << "\n";
    std::cout << "   Average salary: $" << avg_salary << "\n";
    if (max_salary_person) {
        std::cout << "   Highest paid: " << max_salary_person->name 
                  << " ($" << max_salary_person->salary << ")\n";
    }
    
    // =========================================================================
    // 7. Group by
    // =========================================================================
    std::cout << "\n7. Group by department\n";
    auto by_dept = people | rpp::group_by(&Person::department);
    
    for (const auto& [dept, members] : by_dept) {
        std::cout << "   " << dept << ": ";
        for (const auto& p : members) {
            std::cout << p.name << " ";
        }
        std::cout << "\n";
    }
    
    // =========================================================================
    // 8. Select (projection)
    // =========================================================================
    std::cout << "\n8. Select name and salary\n";
    auto name_salary = people 
        | rpp::select(&Person::name, &Person::salary)
        | rpp::to_vector;
    
    for (const auto& [name, salary] : name_salary) {
        std::cout << "   " << name << ": $" << salary << "\n";
    }
    
    // =========================================================================
    // 9. Distinct
    // =========================================================================
    std::cout << "\n9. Distinct departments\n";
    auto unique_depts = people 
        | rpp::distinct(&Person::department);
    
    for (const auto& p : unique_depts) {
        std::cout << "   " << p.department << "\n";
    }
    
    // =========================================================================
    // 10. Join
    // =========================================================================
    std::cout << "\n10. Inner Join: People with Department locations\n";
    auto joined = people 
        | rpp::join(departments, &Person::department, &Department::name,
            [](const Person& p, const Department& d) {
                return std::make_tuple(p.name, p.department, d.location);
            });
    
    for (const auto& [name, dept, loc] : joined) {
        std::cout << "   " << name << " works in " << dept << " at " << loc << "\n";
    }
    
    // =========================================================================
    // 11. Set operations
    // =========================================================================
    std::cout << "\n11. Set operations\n";
    std::vector<int> set1 = {1, 2, 3, 4, 5};
    std::vector<int> set2 = {4, 5, 6, 7, 8};
    
    auto union_result = set1 | rpp::union_(set2);
    auto intersect_result = set1 | rpp::intersect(set2);
    auto except_result = set1 | rpp::except(set2);
    
    std::cout << "   Union: ";
    for (int x : union_result) std::cout << x << " ";
    std::cout << "\n   Intersect: ";
    for (int x : intersect_result) std::cout << x << " ";
    std::cout << "\n   Except: ";
    for (int x : except_result) std::cout << x << " ";
    std::cout << "\n";
    
    // =========================================================================
    // 12. Window functions
    // =========================================================================
    std::cout << "\n12. Window: Row number by department\n";
    auto with_row_nums = people 
        | rpp::window(&Person::department, &Person::salary)
            .select(rpp::window_fns::row_number, rpp::window_fns::rank);
    
    for (const auto& [person, window_vals] : with_row_nums) {
        auto [row_num, rank] = window_vals;
        std::cout << "   " << person.department << " - " << person.name 
                  << " (row=" << row_num << ", rank=" << rank << ")\n";
    }
    
    // =========================================================================
    // 13. Natural sorting
    // =========================================================================
    std::cout << "\n13. Natural sort (numeric-aware)\n";
    std::vector<std::string> files = {"file1.txt", "file10.txt", "file2.txt", "file20.txt"};
    
    auto natural_sorted = files | rpp::natural_sort();
    
    for (const auto& f : natural_sorted) {
        std::cout << "   " << f << "\n";
    }
    
    // =========================================================================
    // 14. Pipeline composition (sort + take now works!)
    // =========================================================================
    std::cout << "\n14. Complex pipeline: Top 3 highest paid engineers\n";
    auto top_engineers = people 
        | rpp::filter([](const Person& p) { return p.department == "Engineering"; })
        | rpp::sort_by(&Person::salary).desc()
        | ::ranges::views::take(3);
    
    for (const auto& p : top_engineers) {
        std::cout << "   " << p.name << " - $" << p.salary << "\n";
    }
    
    // Also demonstrate adapter composition
    std::cout << "\n14b. Adapter composition: filter | sort | take\n";
    auto top_sales = people 
        | rpp::filter([](const Person& p) { return p.department == "Sales"; })
        | rpp::sort_by(&Person::salary).desc()
        | ::ranges::views::take(2);
    
    for (const auto& p : top_sales) {
        std::cout << "   " << p.name << " - $" << p.salary << "\n";
    }
    
    // =========================================================================
    // 15. Statistics
    // =========================================================================
    std::cout << "\n15. Statistics\n";
    auto salary_stddev = people | rpp::stddev(&Person::salary);
    auto salary_median = people | rpp::median(&Person::salary);
    auto salary_p90 = people | rpp::percentile(0.9, &Person::salary);
    
    std::cout << "   Salary std dev: $" << salary_stddev << "\n";
    std::cout << "   Salary median: $" << salary_median << "\n";
    std::cout << "   Salary 90th percentile: $" << salary_p90 << "\n";
    
    // =========================================================================
    // 16. Transform and filter_map
    // =========================================================================
    std::cout << "\n16. Transform: Double salaries\n";
    auto doubled = people 
        | rpp::map([](const Person& p) { 
            return Person{p.name, p.age, p.department, p.salary * 2}; 
          })
        | rpp::take(3)
        | rpp::to_vector;
    
    for (const auto& p : doubled) {
        std::cout << "   " << p.name << " - $" << p.salary << "\n";
    }
    
    std::cout << "\n=== Done ===\n";
    return 0;
}
