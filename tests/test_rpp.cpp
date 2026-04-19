#include <gtest/gtest.h>
#include <rpp/rpp.hpp>
#include <vector>
#include <string>
#include <list>
#include <deque>
#include <set>
#include <numeric>

// ============================================================================
// Test data
// ============================================================================

struct Person {
    std::string name;
    int age;
    std::string department;
    double salary;
    
    bool operator==(const Person& o) const = default;
};

static std::vector<Person> people() {
    return {
        {"Alice", 30, "Engineering", 80000},
        {"Bob", 25, "Engineering", 70000},
        {"Charlie", 35, "Sales", 60000},
        {"Diana", 28, "Engineering", 75000},
        {"Eve", 32, "Sales", 65000},
        {"Frank", 40, "HR", 55000},
        {"Grace", 27, "HR", 52000},
    };
}

// ============================================================================
// Phase 1.1: materialize
// ============================================================================

TEST(RppMaterialize, BasicMaterialize) {
    auto v = std::vector{1, 2, 3, 4, 5};
    auto result = v | rpp::filter([](int x) { return x > 2; }) | rpp::materialize;
    // result is owning_view, should be iterable and support size()
    EXPECT_EQ(result.size(), 3u);
    std::vector<int> collected(result.begin(), result.end());
    EXPECT_EQ(collected, (std::vector{3, 4, 5}));
}

TEST(RppMaterialize, EmptyRange) {
    auto v = std::vector<int>{};
    auto result = v | rpp::materialize;
    EXPECT_EQ(result.size(), 0u);
    EXPECT_TRUE(result.empty());
}

TEST(RppMaterialize, SortThenTakePipeline) {
    // This is THE key use case: sort returns owning_view, then take should work
    auto p = people();
    auto sorted = p | rpp::sort_by(&Person::salary).desc();
    // sorted is owning_view<vector<Person>> — take from range-v3 should work
    auto top3 = sorted | ::ranges::views::take(3) | rpp::to_vector;
    ASSERT_EQ(top3.size(), 3u);
    EXPECT_EQ(top3[0].name, "Alice");
    EXPECT_EQ(top3[1].name, "Diana");
    EXPECT_EQ(top3[2].name, "Bob");
}

TEST(RppMaterialize, MaterializeThenContinue) {
    auto v = std::vector{5, 3, 1, 4, 2};
    auto result = v 
        | rpp::filter([](int x) { return x > 1; })
        | rpp::materialize
        | rpp::sort()
        | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{2, 3, 4, 5}));
}

// ============================================================================
// Phase 1.3: from
// ============================================================================

TEST(RppFrom, FromLvalue) {
    std::vector<int> v = {1, 2, 3};
    auto result = rpp::from(v) | rpp::filter([](int x) { return x > 1; }) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{2, 3}));
}

TEST(RppFrom, FromRvalue) {
    auto result = rpp::from(std::vector{10, 20, 30}) 
        | rpp::filter([](int x) { return x > 15; }) 
        | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{20, 30}));
}

TEST(RppFrom, FromEmpty) {
    auto result = rpp::from(std::vector<int>{}) | rpp::to_vector;
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// Phase 2.1: for_each
// ============================================================================

TEST(RppForEach, BasicForEach) {
    auto v = std::vector{1, 2, 3, 4, 5};
    int sum = 0;
    v | rpp::for_each([&sum](int x) { sum += x; });
    EXPECT_EQ(sum, 15);
}

TEST(RppForEach, ForEachAfterFilter) {
    auto v = std::vector{1, 2, 3, 4, 5};
    std::vector<int> collected;
    v | rpp::filter([](int x) { return x % 2 == 0; })
      | rpp::for_each([&collected](int x) { collected.push_back(x); });
    EXPECT_EQ(collected, (std::vector{2, 4}));
}

TEST(RppForEach, ForEachEmpty) {
    auto v = std::vector<int>{};
    int count = 0;
    v | rpp::for_each([&count](int) { ++count; });
    EXPECT_EQ(count, 0);
}

// ============================================================================
// Phase 2.2: join_str
// ============================================================================

TEST(RppJoinStr, BasicJoin) {
    auto v = std::vector<std::string>{"Alice", "Bob", "Charlie"};
    auto result = v | rpp::join_str(", ");
    EXPECT_EQ(result, "Alice, Bob, Charlie");
}

TEST(RppJoinStr, IntJoin) {
    auto v = std::vector{1, 2, 3};
    auto result = v | rpp::join_str("-");
    EXPECT_EQ(result, "1-2-3");
}

TEST(RppJoinStr, SingleElement) {
    auto v = std::vector<std::string>{"only"};
    auto result = v | rpp::join_str(", ");
    EXPECT_EQ(result, "only");
}

TEST(RppJoinStr, EmptyRange) {
    auto v = std::vector<std::string>{};
    auto result = v | rpp::join_str(", ");
    EXPECT_EQ(result, "");
}

TEST(RppJoinStr, AfterPluck) {
    auto p = people();
    auto result = p 
        | rpp::filter([](const Person& p) { return p.department == "HR"; })
        | rpp::pluck(&Person::name)
        | rpp::join_str(" & ");
    EXPECT_EQ(result, "Frank & Grace");
}

// ============================================================================
// Phase 2.3: to<Container>
// ============================================================================

TEST(RppTo, ToList) {
    auto v = std::vector{1, 2, 3};
    auto result = v | rpp::to<std::list>();
    EXPECT_EQ(result.size(), 3u);
    EXPECT_EQ(result.front(), 1);
    EXPECT_EQ(result.back(), 3);
}

TEST(RppTo, ToDeque) {
    auto v = std::vector{10, 20, 30};
    auto result = v | rpp::to<std::deque>();
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], 10);
    EXPECT_EQ(result[2], 30);
}

TEST(RppTo, ToSet) {
    auto v = std::vector{3, 1, 2, 3, 1};
    auto result = v | rpp::to<std::set>();
    EXPECT_EQ(result.size(), 3u);
    EXPECT_TRUE(result.contains(1));
    EXPECT_TRUE(result.contains(2));
    EXPECT_TRUE(result.contains(3));
}

// ============================================================================
// Phase 2.4: to_unordered_map
// ============================================================================

TEST(RppToUnorderedMap, BasicMap) {
    auto p = people();
    auto result = p | rpp::to_unordered_map(&Person::name, &Person::salary);
    EXPECT_DOUBLE_EQ(result["Alice"], 80000);
    EXPECT_DOUBLE_EQ(result["Bob"], 70000);
    EXPECT_EQ(result.size(), 7u);
}

TEST(RppToUnorderedMap, IdentityValue) {
    auto p = people();
    auto result = p | rpp::to_unordered_map(&Person::name);
    EXPECT_EQ(result.size(), 7u);
    EXPECT_EQ(result["Alice"].age, 30);
}

// ============================================================================
// Phase 2.5: format_table
// ============================================================================

TEST(RppFormatTable, BasicTable) {
    auto data = std::vector<std::tuple<std::string, int>>{
        {"Alice", 30}, {"Bob", 25}
    };
    auto table = data | rpp::format_table("Name", "Age");
    EXPECT_FALSE(table.empty());
    EXPECT_NE(table.find("Alice"), std::string::npos);
    EXPECT_NE(table.find("Bob"), std::string::npos);
    EXPECT_NE(table.find("Name"), std::string::npos);
    EXPECT_NE(table.find("Age"), std::string::npos);
}

TEST(RppFormatTable, NoHeaders) {
    auto data = std::vector<std::tuple<int, int>>{{1, 2}, {3, 4}};
    auto table = data | rpp::format_table();
    EXPECT_FALSE(table.empty());
    EXPECT_NE(table.find("1"), std::string::npos);
}

TEST(RppFormatTable, EmptyData) {
    auto data = std::vector<std::tuple<int, int>>{};
    auto table = data | rpp::format_table("A", "B");
    // Should still have header
    EXPECT_NE(table.find("A"), std::string::npos);
}

// ============================================================================
// Phase 3.1: group_aggregate
// ============================================================================

TEST(RppGroupAggregate, SumByDepartment) {
    auto p = people();
    auto result = p | rpp::group_aggregate(
        &Person::department,
        rpp::sum(&Person::salary),
        rpp::count
    );
    
    // Result is vector<tuple<string, double, ptrdiff_t>>
    ASSERT_EQ(result.size(), 3u);
    
    // Find Engineering group
    for (const auto& [dept, total, cnt] : result) {
        if (dept == "Engineering") {
            EXPECT_DOUBLE_EQ(total, 225000);
            EXPECT_EQ(cnt, 3);
        } else if (dept == "Sales") {
            EXPECT_DOUBLE_EQ(total, 125000);
            EXPECT_EQ(cnt, 2);
        } else if (dept == "HR") {
            EXPECT_DOUBLE_EQ(total, 107000);
            EXPECT_EQ(cnt, 2);
        }
    }
}

TEST(RppGroupAggregate, AvgByDepartment) {
    auto p = people();
    auto result = p | rpp::group_aggregate(
        &Person::department,
        rpp::avg(&Person::salary)
    );
    
    for (const auto& [dept, avg_sal] : result) {
        if (dept == "Engineering") {
            EXPECT_NEAR(avg_sal, 75000, 1);
        }
    }
}

// ============================================================================
// Phase 3.2: having
// ============================================================================

TEST(RppHaving, FilterGroups) {
    auto p = people();
    auto result = p | rpp::group_aggregate(
        &Person::department,
        rpp::count
    ) | rpp::having([](const auto& row) {
        return std::get<1>(row) > 2;
    });
    
    // Only Engineering has >2 members
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(std::get<0>(result[0]), "Engineering");
}

TEST(RppHaving, NoMatch) {
    auto v = std::vector{1, 2, 3};
    auto result = v | rpp::having([](int x) { return x > 100; });
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// Phase 4.1: sliding_window
// ============================================================================

TEST(RppSlidingWindow, BasicWindow) {
    auto v = std::vector{1, 2, 3, 4, 5};
    auto result = v | rpp::sliding_window(3);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], (std::vector{1, 2, 3}));
    EXPECT_EQ(result[1], (std::vector{2, 3, 4}));
    EXPECT_EQ(result[2], (std::vector{3, 4, 5}));
}

TEST(RppSlidingWindow, WindowSizeEqualsRange) {
    auto v = std::vector{1, 2, 3};
    auto result = v | rpp::sliding_window(3);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0], (std::vector{1, 2, 3}));
}

TEST(RppSlidingWindow, WindowSizeLargerThanRange) {
    auto v = std::vector{1, 2};
    auto result = v | rpp::sliding_window(5);
    EXPECT_TRUE(result.empty());
}

TEST(RppSlidingWindow, WindowSizeOne) {
    auto v = std::vector{10, 20, 30};
    auto result = v | rpp::sliding_window(1);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], (std::vector{10}));
    EXPECT_EQ(result[1], (std::vector{20}));
    EXPECT_EQ(result[2], (std::vector{30}));
}

TEST(RppSlidingWindow, MovingAverage) {
    auto v = std::vector{1.0, 2.0, 3.0, 4.0, 5.0};
    auto windows = v | rpp::sliding_window(3);
    
    std::vector<double> averages;
    for (const auto& w : windows) {
        double sum = 0;
        for (double x : w) sum += x;
        averages.push_back(sum / w.size());
    }
    ASSERT_EQ(averages.size(), 3u);
    EXPECT_DOUBLE_EQ(averages[0], 2.0);
    EXPECT_DOUBLE_EQ(averages[1], 3.0);
    EXPECT_DOUBLE_EQ(averages[2], 4.0);
}

// ============================================================================
// Phase 1.4: lag/lead with arbitrary offset (window functions)
// ============================================================================

TEST(RppWindow, LagOffset1) {
    auto v = std::vector{10, 20, 30, 40, 50};
    auto result = v 
        | rpp::window(std::identity{})
            .select(rpp::window_fns::lag(std::identity{}, 1, 0));
    
    ASSERT_EQ(result.size(), 5u);
    EXPECT_EQ(std::get<0>(result[0].second), 0);   // no prev
    EXPECT_EQ(std::get<0>(result[1].second), 10);
    EXPECT_EQ(std::get<0>(result[2].second), 20);
}

TEST(RppWindow, LagOffset2) {
    auto v = std::vector{10, 20, 30, 40, 50};
    auto result = v 
        | rpp::window(std::identity{})
            .select(rpp::window_fns::lag(std::identity{}, 2, -1));
    
    ASSERT_EQ(result.size(), 5u);
    EXPECT_EQ(std::get<0>(result[0].second), -1);  // offset 2: no lag
    EXPECT_EQ(std::get<0>(result[1].second), -1);  // offset 2: no lag
    EXPECT_EQ(std::get<0>(result[2].second), 10);
    EXPECT_EQ(std::get<0>(result[3].second), 20);
    EXPECT_EQ(std::get<0>(result[4].second), 30);
}

TEST(RppWindow, LeadOffset2) {
    auto v = std::vector{10, 20, 30, 40, 50};
    auto result = v 
        | rpp::window(std::identity{})
            .select(rpp::window_fns::lead(std::identity{}, 2, -1));
    
    ASSERT_EQ(result.size(), 5u);
    EXPECT_EQ(std::get<0>(result[0].second), 30);
    EXPECT_EQ(std::get<0>(result[1].second), 40);
    EXPECT_EQ(std::get<0>(result[2].second), 50);
    EXPECT_EQ(std::get<0>(result[3].second), -1);  // no lead
    EXPECT_EQ(std::get<0>(result[4].second), -1);  // no lead
}

// ============================================================================
// Existing features regression tests
// ============================================================================

TEST(RppFilter, BasicFilter) {
    auto v = std::vector{1, 2, 3, 4, 5};
    auto result = v | rpp::filter([](int x) { return x > 3; }) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{4, 5}));
}

TEST(RppFilter, EmptyResult) {
    auto v = std::vector{1, 2, 3};
    auto result = v | rpp::filter([](int) { return false; }) | rpp::to_vector;
    EXPECT_TRUE(result.empty());
}

TEST(RppFilter, AllPass) {
    auto v = std::vector{1, 2, 3};
    auto result = v | rpp::filter([](int) { return true; }) | rpp::to_vector;
    EXPECT_EQ(result, v);
}

TEST(RppFilters, Eq) {
    auto p = people();
    auto result = p | rpp::filters::eq(&Person::department, std::string("HR")) | rpp::to_vector;
    EXPECT_EQ(result.size(), 2u);
}

TEST(RppFilters, Between) {
    auto p = people();
    auto result = p | rpp::filters::between(&Person::age, 28, 35) | rpp::to_vector;
    // Alice(30), Diana(28), Charlie(35), Eve(32) 
    EXPECT_EQ(result.size(), 4u);
}

TEST(RppFilters, In) {
    auto p = people();
    auto result = p | rpp::filters::in(&Person::name, 
        std::vector<std::string>{"Alice", "Bob"}) | rpp::to_vector;
    EXPECT_EQ(result.size(), 2u);
}

TEST(RppFilters, Contains) {
    auto p = people();
    auto result = p | rpp::filters::contains(&Person::name, std::string("li")) | rpp::to_vector;
    // Alice, Charlie
    EXPECT_EQ(result.size(), 2u);
}

TEST(RppFilters, StartsWith) {
    auto p = people();
    auto result = p | rpp::filters::starts_with(&Person::name, "A") | rpp::to_vector;
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].name, "Alice");
}

TEST(RppSort, BasicSort) {
    auto v = std::vector{5, 3, 1, 4, 2};
    auto result = v | rpp::sort() | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{1, 2, 3, 4, 5}));
}

TEST(RppSort, SortByDesc) {
    auto p = people();
    auto result = p | rpp::sort_by(&Person::salary).desc() | rpp::to_vector;
    EXPECT_EQ(result[0].name, "Alice");
    EXPECT_EQ(result.back().name, "Grace");
}

TEST(RppSort, MultiSort) {
    auto p = people();
    auto result = p | rpp::multi_sort(
        rpp::sort_by(&Person::department),
        rpp::sort_by(&Person::salary).desc()
    ) | rpp::to_vector;
    // Engineering first (alpha), sorted by salary desc within
    EXPECT_EQ(result[0].department, "Engineering");
    EXPECT_EQ(result[0].name, "Alice");
}

TEST(RppSort, NaturalSort) {
    auto v = std::vector<std::string>{"file1.txt", "file10.txt", "file2.txt", "file20.txt"};
    auto result = v | rpp::natural_sort() | rpp::to_vector;
    EXPECT_EQ(result[0], "file1.txt");
    EXPECT_EQ(result[1], "file2.txt");
    EXPECT_EQ(result[2], "file10.txt");
    EXPECT_EQ(result[3], "file20.txt");
}

TEST(RppSort, TopN) {
    auto v = std::vector{5, 3, 1, 4, 2};
    auto result = v | rpp::top_n(3) | rpp::to_vector;
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], 5);
    EXPECT_EQ(result[1], 4);
    EXPECT_EQ(result[2], 3);
}

TEST(RppSort, BottomN) {
    auto v = std::vector{5, 3, 1, 4, 2};
    auto result = v | rpp::bottom_n(2) | rpp::to_vector;
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], 1);
    EXPECT_EQ(result[1], 2);
}

TEST(RppSelect, MultiColumn) {
    auto p = people();
    auto result = p | rpp::select(&Person::name, &Person::salary) | rpp::to_vector;
    ASSERT_EQ(result.size(), 7u);
    EXPECT_EQ(std::get<0>(result[0]), "Alice");
    EXPECT_DOUBLE_EQ(std::get<1>(result[0]), 80000);
}

TEST(RppSelect, Pluck) {
    auto p = people();
    auto names = p | rpp::pluck(&Person::name) | rpp::to_vector;
    ASSERT_EQ(names.size(), 7u);
    EXPECT_EQ(names[0], "Alice");
}

TEST(RppSelect, CaseWhen) {
    auto p = people();
    auto proj = rpp::case_when(
        [](const Person& p) { return p.salary >= 70000; },
        [](const Person&) { return std::string("High"); },
        [](const Person&) { return std::string("Low"); }
    );
    auto result = p | rpp::pluck(proj) | rpp::to_vector;
    EXPECT_EQ(result[0], "High");   // Alice: 80000
    EXPECT_EQ(result[2], "Low");    // Charlie: 60000
}

TEST(RppDistinct, BasicDistinct) {
    auto v = std::vector{1, 2, 2, 3, 3, 3, 1};
    auto result = v | rpp::distinct() | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{1, 2, 3}));
}

TEST(RppDistinct, DistinctByProj) {
    auto p = people();
    auto result = p | rpp::distinct(&Person::department) | rpp::to_vector;
    EXPECT_EQ(result.size(), 3u);
}

TEST(RppDistinct, DistinctCount) {
    auto v = std::vector{1, 2, 2, 3, 3, 3};
    auto result = v | rpp::distinct_count();
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].second, 1u);
    EXPECT_EQ(result[1].second, 2u);
    EXPECT_EQ(result[2].second, 3u);
}

TEST(RppAggregate, First) {
    auto v = std::vector{10, 20, 30};
    auto result = v | rpp::first;
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 10);
}

TEST(RppAggregate, Last) {
    auto v = std::vector{10, 20, 30};
    auto result = v | rpp::last;
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 30);
}

TEST(RppAggregate, FirstEmpty) {
    auto v = std::vector<int>{};
    auto result = v | rpp::first;
    EXPECT_FALSE(result.has_value());
}

TEST(RppAggregate, Count) {
    auto p = people();
    auto n = p | rpp::count;
    EXPECT_EQ(n, 7);
}

TEST(RppAggregate, Sum) {
    auto v = std::vector{1, 2, 3, 4, 5};
    auto s = v | rpp::sum();
    EXPECT_EQ(s, 15);
}

TEST(RppAggregate, SumProj) {
    auto p = people();
    auto total = p | rpp::sum(&Person::salary);
    EXPECT_DOUBLE_EQ(total, 457000);
}

TEST(RppAggregate, Avg) {
    auto v = std::vector{10, 20, 30};
    auto a = v | rpp::avg();
    EXPECT_DOUBLE_EQ(a, 20.0);
}

TEST(RppAggregate, MaxBy) {
    auto p = people();
    auto highest = p | rpp::max_by(&Person::salary);
    ASSERT_TRUE(highest.has_value());
    EXPECT_EQ(highest->name, "Alice");
}

TEST(RppAggregate, MinBy) {
    auto p = people();
    auto lowest = p | rpp::min_by(&Person::salary);
    ASSERT_TRUE(lowest.has_value());
    EXPECT_EQ(lowest->name, "Grace");
}

TEST(RppAggregate, Median) {
    auto v = std::vector{1, 3, 5, 7, 9};
    auto m = v | rpp::median();
    EXPECT_DOUBLE_EQ(m, 5.0);
}

TEST(RppAggregate, MedianEven) {
    auto v = std::vector{1, 3, 5, 7};
    auto m = v | rpp::median();
    EXPECT_DOUBLE_EQ(m, 4.0);
}

TEST(RppAggregate, Variance) {
    auto v = std::vector{2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    auto var = v | rpp::variance();
    EXPECT_NEAR(var, 4.0, 0.01);
}

TEST(RppAggregate, Stddev) {
    auto v = std::vector{2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    auto sd = v | rpp::stddev();
    EXPECT_NEAR(sd, 2.0, 0.01);
}

TEST(RppAggregate, Percentile) {
    auto v = std::vector{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto p90 = v | rpp::percentile(0.9);
    EXPECT_NEAR(p90, 9, 1);
}

TEST(RppAggregate, GroupBy) {
    auto p = people();
    auto groups = p | rpp::group_by(&Person::department);
    EXPECT_EQ(groups.size(), 3u);
    EXPECT_EQ(groups["Engineering"].size(), 3u);
    EXPECT_EQ(groups["Sales"].size(), 2u);
    EXPECT_EQ(groups["HR"].size(), 2u);
}

TEST(RppAggregate, CountBy) {
    auto p = people();
    auto counts = p | rpp::count_by(&Person::department);
    EXPECT_EQ(counts["Engineering"], 3u);
}

TEST(RppAggregate, Fold) {
    auto v = std::vector{1, 2, 3, 4};
    auto product = v | rpp::fold(1, std::multiplies<>{});
    EXPECT_EQ(product, 24);
}

TEST(RppAggregate, Reduce) {
    auto v = std::vector{1, 2, 3, 4};
    auto sum = v | rpp::reduce(std::plus<>{});
    ASSERT_TRUE(sum.has_value());
    EXPECT_EQ(*sum, 10);
}

TEST(RppAggregate, ReduceEmpty) {
    auto v = std::vector<int>{};
    auto result = v | rpp::reduce(std::plus<>{});
    EXPECT_FALSE(result.has_value());
}

TEST(RppAggregate, Take) {
    auto v = std::vector{1, 2, 3, 4, 5};
    auto result = v | rpp::take(3) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{1, 2, 3}));
}

TEST(RppAggregate, Drop) {
    auto v = std::vector{1, 2, 3, 4, 5};
    auto result = v | rpp::drop(2) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{3, 4, 5}));
}

TEST(RppAggregate, Nth) {
    auto v = std::vector{10, 20, 30, 40};
    auto val = v | rpp::nth(2);
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 30);
}

TEST(RppAggregate, Exists) {
    auto v = std::vector{1, 2, 3};
    EXPECT_TRUE(v | rpp::exists([](int x) { return x == 2; }));
    EXPECT_FALSE(v | rpp::exists([](int x) { return x == 99; }));
}

TEST(RppAggregate, All) {
    auto v = std::vector{2, 4, 6};
    EXPECT_TRUE(v | rpp::all([](int x) { return x % 2 == 0; }));
    EXPECT_FALSE(v | rpp::all([](int x) { return x > 3; }));
}

TEST(RppAggregate, None) {
    auto v = std::vector{1, 3, 5};
    EXPECT_TRUE(v | rpp::none([](int x) { return x % 2 == 0; }));
}

TEST(RppAggregate, ToMap) {
    auto p = people();
    auto map = p | rpp::to_map(&Person::name, &Person::age);
    EXPECT_EQ(map["Alice"], 30);
    EXPECT_EQ(map["Bob"], 25);
}

TEST(RppTransform, Map) {
    auto v = std::vector{1, 2, 3};
    auto result = v | rpp::map([](int x) { return x * 10; }) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{10, 20, 30}));
}

TEST(RppTransform, FlatMap) {
    auto v = std::vector{1, 2, 3};
    auto result = v | rpp::flat_map([](int x) { 
        return std::vector{x, x * 10}; 
    }) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{1, 10, 2, 20, 3, 30}));
}

TEST(RppTransform, FilterMap) {
    auto v = std::vector{1, 2, 3, 4, 5};
    auto result = v | rpp::filter_map([](int x) -> std::optional<int> {
        if (x % 2 == 0) return x * 10;
        return std::nullopt;
    }) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{20, 40}));
}

TEST(RppTransform, Scan) {
    auto v = std::vector{1, 2, 3, 4};
    auto result = v | rpp::scan(0, std::plus<>{}) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{1, 3, 6, 10}));
}

TEST(RppTransform, Tap) {
    auto v = std::vector{1, 2, 3};
    int sum = 0;
    auto result = v | rpp::tap([&sum](int x) { sum += x; }) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{1, 2, 3}));
    EXPECT_EQ(sum, 6);
}

TEST(RppJoin, InnerJoin) {
    auto p = people();
    struct Dept { std::string name; std::string loc; };
    auto depts = std::vector<Dept>{
        {"Engineering", "A"}, {"Sales", "B"}, {"HR", "C"}
    };
    auto result = p | rpp::join(depts, &Person::department, &Dept::name);
    EXPECT_EQ(result.size(), 7u);
}

TEST(RppJoin, LeftJoin) {
    auto p = std::vector<std::pair<int, std::string>>{
        {1, "Alice"}, {2, "Bob"}, {3, "Charlie"}
    };
    auto scores = std::vector<std::pair<int, int>>{{1, 90}, {3, 85}};
    
    auto result = p | rpp::left_join(scores,
        [](const auto& p) { return p.first; },
        [](const auto& s) { return s.first; });
    
    EXPECT_EQ(result.size(), 3u);
}

TEST(RppSetOps, Union) {
    auto a = std::vector{1, 2, 3};
    auto b = std::vector{3, 4, 5};
    auto result = a | rpp::union_(b);
    EXPECT_EQ(result.size(), 5u);
}

TEST(RppSetOps, Intersect) {
    auto a = std::vector{1, 2, 3, 4};
    auto b = std::vector{3, 4, 5, 6};
    auto result = a | rpp::intersect(b);
    EXPECT_EQ(result.size(), 2u);
}

TEST(RppSetOps, Except) {
    auto a = std::vector{1, 2, 3, 4};
    auto b = std::vector{3, 4, 5};
    auto result = a | rpp::except(b);
    EXPECT_EQ(result.size(), 2u);
}

TEST(RppSetOps, SymmetricDifference) {
    auto a = std::vector{1, 2, 3};
    auto b = std::vector{2, 3, 4};
    auto result = a | rpp::symmetric_difference(b);
    // 1 and 4
    EXPECT_EQ(result.size(), 2u);
}

TEST(RppWindow, RowNumber) {
    auto v = std::vector{30, 10, 20};
    auto result = v | rpp::window(std::identity{})
        .select(rpp::window_fns::row_number);
    ASSERT_EQ(result.size(), 3u);
    // Ordered by identity: 10, 20, 30
    EXPECT_EQ(std::get<0>(result[0].second), 1u);
    EXPECT_EQ(std::get<0>(result[1].second), 2u);
    EXPECT_EQ(std::get<0>(result[2].second), 3u);
}

TEST(RppWindow, RankWithTies) {
    auto v = std::vector{10, 20, 20, 30};
    auto result = v | rpp::window(std::identity{})
        .select(rpp::window_fns::rank, rpp::window_fns::dense_rank);
    
    // 10: rank=1,dense=1; 20: rank=2,dense=2; 20: rank=2,dense=2; 30: rank=4,dense=3
    EXPECT_EQ(std::get<0>(result[0].second), 1u);
    EXPECT_EQ(std::get<0>(result[3].second), 4u);
    EXPECT_EQ(std::get<1>(result[3].second), 3u);
}

// ============================================================================
// Complex pipeline tests
// ============================================================================

TEST(RppPipeline, FilterSortTake) {
    auto p = people();
    auto result = p
        | rpp::filter([](const Person& p) { return p.department == "Engineering"; })
        | rpp::sort_by(&Person::salary).desc()
        | rpp::to_vector;
    
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].name, "Alice");
    EXPECT_EQ(result[1].name, "Diana");
    EXPECT_EQ(result[2].name, "Bob");
}

TEST(RppPipeline, FilterSortSelectJoinStr) {
    auto p = people();
    auto result = p
        | rpp::filter([](const Person& p) { return p.salary >= 65000; })
        | rpp::sort_by(&Person::salary).desc()
        | rpp::pluck(&Person::name)
        | rpp::join_str(", ");
    
    EXPECT_EQ(result, "Alice, Diana, Bob, Eve");
}

TEST(RppPipeline, DistinctSortJoinStr) {
    auto v = std::vector<std::string>{"banana", "apple", "cherry", "apple", "banana"};
    auto result = v 
        | rpp::distinct() 
        | rpp::sort()
        | rpp::join_str(" | ");
    EXPECT_EQ(result, "apple | banana | cherry");
}

TEST(RppPipeline, GroupAggregateHavingSort) {
    auto p = people();
    auto result = p
        | rpp::group_aggregate(&Person::department, rpp::avg(&Person::salary))
        | rpp::having([](const auto& row) { return std::get<1>(row) > 60000; })
        | rpp::to_vector; // should be a no-op since having already returns vector
    
    // Only Engineering (75000 avg) qualifies, Sales is 62500
    // Actually: Engineering=75000, Sales=62500, HR=53500
    // >60000: Engineering, Sales
    EXPECT_EQ(result.size(), 2u);
}

TEST(RppPipeline, FromRvalueFullPipeline) {
    auto result = rpp::from(std::vector{5, 3, 1, 4, 2})
        | rpp::filter([](int x) { return x > 2; })
        | rpp::sort()
        | rpp::map([](int x) { return x * 100; })
        | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{300, 400, 500}));
}

// ============================================================================
// Phase 4.2: owning_view unique_ptr optimization
// ============================================================================

TEST(RppOwningView, MoveSemantics) {
    auto v = std::vector{1, 2, 3};
    auto ov = rpp::owning_view<std::vector<int>>{std::move(v)};
    EXPECT_EQ(ov.size(), 3u);
    
    auto ov2 = std::move(ov);
    EXPECT_EQ(ov2.size(), 3u);
    EXPECT_EQ(ov2[0], 1);
}

TEST(RppOwningView, CopySemantics) {
    auto ov = rpp::owning_view<std::vector<int>>{std::vector{10, 20, 30}};
    auto ov_copy = ov;
    EXPECT_EQ(ov_copy.size(), 3u);
    EXPECT_EQ(ov_copy[1], 20);
    // Modify copy, original unaffected
    ov_copy.container()[0] = 99;
    EXPECT_EQ(ov[0], 10);
}

TEST(RppOwningView, PipelineContinuation) {
    auto result = std::vector{5, 3, 1, 4, 2}
        | rpp::sort()
        | rpp::filter([](int x) { return x > 2; })
        | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{3, 4, 5}));
}

// ============================================================================
// Phase 5.1: zip_with
// ============================================================================

TEST(RppZipWith, BasicZipWith) {
    auto a = std::vector{1, 2, 3};
    auto b = std::vector{10, 20, 30};
    auto result = a | rpp::zip_with(b, [](int x, int y) { return x + y; });
    EXPECT_EQ(result, (std::vector{11, 22, 33}));
}

TEST(RppZipWith, DifferentLengths) {
    auto a = std::vector{1, 2, 3, 4, 5};
    auto b = std::vector{10, 20};
    auto result = a | rpp::zip_with(b, std::plus<>{});
    EXPECT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], 11);
    EXPECT_EQ(result[1], 22);
}

TEST(RppZipWith, EmptyRange) {
    auto a = std::vector<int>{};
    auto b = std::vector{1, 2, 3};
    auto result = a | rpp::zip_with(b, std::plus<>{});
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// Phase 5.2: enumerate_map
// ============================================================================

TEST(RppEnumerateMap, Basic) {
    auto v = std::vector<std::string>{"a", "b", "c"};
    auto result = v | rpp::enumerate_map([](std::size_t i, const std::string& s) {
        return std::to_string(i) + ":" + s;
    });
    EXPECT_EQ(result, (std::vector<std::string>{"0:a", "1:b", "2:c"}));
}

TEST(RppEnumerateMap, Empty) {
    auto v = std::vector<int>{};
    auto result = v | rpp::enumerate_map([](std::size_t i, int x) { return static_cast<int>(i) + x; });
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// Phase 5.3: batch
// ============================================================================

TEST(RppBatch, SumBatches) {
    auto v = std::vector{1, 2, 3, 4, 5, 6};
    auto result = v | rpp::batch(3, [](const std::vector<int>& chunk) {
        int s = 0;
        for (int x : chunk) s += x;
        return s;
    });
    EXPECT_EQ(result, (std::vector{6, 15}));
}

TEST(RppBatch, UnevenBatch) {
    auto v = std::vector{1, 2, 3, 4, 5};
    auto result = v | rpp::batch(2, [](const std::vector<int>& chunk) {
        return chunk.size();
    });
    // [1,2], [3,4], [5]
    EXPECT_EQ(result, (std::vector<std::size_t>{2u, 2u, 1u}));
}

TEST(RppBatch, EmptyRange) {
    auto v = std::vector<int>{};
    auto result = v | rpp::batch(3, [](const std::vector<int>& chunk) {
        return static_cast<int>(chunk.size());
    });
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// Phase 5.4: pairwise
// ============================================================================

TEST(RppPairwise, Basic) {
    auto v = std::vector{1, 2, 3, 4};
    auto result = v | rpp::pairwise;
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], (std::pair{1, 2}));
    EXPECT_EQ(result[1], (std::pair{2, 3}));
    EXPECT_EQ(result[2], (std::pair{3, 4}));
}

TEST(RppPairwise, SingleElement) {
    auto v = std::vector{42};
    auto result = v | rpp::pairwise;
    EXPECT_TRUE(result.empty());
}

TEST(RppPairwise, Empty) {
    auto v = std::vector<int>{};
    auto result = v | rpp::pairwise;
    EXPECT_TRUE(result.empty());
}

TEST(RppPairwise, Differences) {
    auto v = std::vector{10, 13, 17, 22};
    auto pairs = v | rpp::pairwise;
    std::vector<int> diffs;
    for (auto& [a, b] : pairs) diffs.push_back(b - a);
    EXPECT_EQ(diffs, (std::vector{3, 4, 5}));
}

// ============================================================================
// Phase 5.5: stride
// ============================================================================

TEST(RppStride, EveryOther) {
    auto v = std::vector{1, 2, 3, 4, 5, 6};
    auto result = v | rpp::stride(2) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{1, 3, 5}));
}

TEST(RppStride, EveryThird) {
    auto v = std::vector{0, 1, 2, 3, 4, 5, 6, 7, 8};
    auto result = v | rpp::stride(3) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{0, 3, 6}));
}

TEST(RppStride, StepOne) {
    auto v = std::vector{1, 2, 3};
    auto result = v | rpp::stride(1) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{1, 2, 3}));
}

TEST(RppStride, Empty) {
    auto v = std::vector<int>{};
    auto result = v | rpp::stride(2) | rpp::to_vector;
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// Phase 5.6: take_while / drop_while
// ============================================================================

TEST(RppTakeWhile, Basic) {
    auto v = std::vector{1, 2, 3, 10, 4, 5};
    auto result = v | rpp::take_while([](int x) { return x < 10; }) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{1, 2, 3}));
}

TEST(RppTakeWhile, AllMatch) {
    auto v = std::vector{1, 2, 3};
    auto result = v | rpp::take_while([](int x) { return x < 10; }) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{1, 2, 3}));
}

TEST(RppTakeWhile, NoneMatch) {
    auto v = std::vector{10, 20, 30};
    auto result = v | rpp::take_while([](int x) { return x < 5; }) | rpp::to_vector;
    EXPECT_TRUE(result.empty());
}

TEST(RppDropWhile, Basic) {
    auto v = std::vector{1, 2, 3, 10, 4, 5};
    auto result = v | rpp::drop_while([](int x) { return x < 10; }) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{10, 4, 5}));
}

TEST(RppDropWhile, AllMatch) {
    auto v = std::vector{1, 2, 3};
    auto result = v | rpp::drop_while([](int x) { return x < 10; }) | rpp::to_vector;
    EXPECT_TRUE(result.empty());
}

TEST(RppDropWhile, NoneMatch) {
    auto v = std::vector{10, 20, 30};
    auto result = v | rpp::drop_while([](int x) { return x < 5; }) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{10, 20, 30}));
}

// ============================================================================
// Phase 5.7: generate / generate_n / iota
// ============================================================================

TEST(RppGenerate, Basic) {
    int counter = 0;
    auto result = rpp::generate([&counter]() { return counter++; }, 5) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{0, 1, 2, 3, 4}));
}

TEST(RppGenerate, Empty) {
    auto result = rpp::generate([]() { return 42; }, 0) | rpp::to_vector;
    EXPECT_TRUE(result.empty());
}

TEST(RppGenerateN, Basic) {
    auto result = rpp::generate_n([](std::size_t i) { return static_cast<int>(i * i); }, 5) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{0, 1, 4, 9, 16}));
}

TEST(RppIota, Range) {
    auto result = rpp::iota(3, 8) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{3, 4, 5, 6, 7}));
}

TEST(RppIota, FromZero) {
    auto result = rpp::iota(5) | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{0, 1, 2, 3, 4}));
}

TEST(RppIota, Empty) {
    auto result = rpp::iota(0) | rpp::to_vector;
    EXPECT_TRUE(result.empty());
}

TEST(RppIota, PipelineCombination) {
    auto result = rpp::iota(1, 11)
        | rpp::filter([](int x) { return x % 2 == 0; })
        | rpp::map([](int x) { return x * x; })
        | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{4, 16, 36, 64, 100}));
}

// ============================================================================
// Phase 5.8: cache
// ============================================================================

TEST(RppCache, Basic) {
    auto v = std::vector{3, 1, 4, 1, 5};
    auto cached = v | rpp::filter([](int x) { return x > 2; }) | rpp::cache;
    // Multi-pass: iterate twice
    auto first_pass = cached | rpp::to_vector;
    auto second_pass = cached | rpp::to_vector;
    EXPECT_EQ(first_pass, (std::vector{3, 4, 5}));
    EXPECT_EQ(second_pass, (std::vector{3, 4, 5}));
}

TEST(RppCache, Empty) {
    auto v = std::vector<int>{};
    auto cached = v | rpp::cache;
    EXPECT_TRUE(cached.empty());
}

// ============================================================================
// Phase 5 complex pipelines
// ============================================================================

TEST(RppPhase5Pipeline, IotaStrideMap) {
    auto result = rpp::iota(0, 20)
        | rpp::stride(3)
        | rpp::map([](int x) { return x * 10; })
        | rpp::to_vector;
    // 0, 3, 6, 9, 12, 15, 18 -> *10
    EXPECT_EQ(result, (std::vector{0, 30, 60, 90, 120, 150, 180}));
}

TEST(RppPhase5Pipeline, GeneratePairwiseBatch) {
    auto result = rpp::generate_n([](std::size_t i) { return static_cast<int>(i + 1); }, 5)
        | rpp::pairwise;
    // [1,2,3,4,5] -> [(1,2),(2,3),(3,4),(4,5)]
    ASSERT_EQ(result.size(), 4u);
    EXPECT_EQ(result[0].first, 1);
    EXPECT_EQ(result[0].second, 2);
    EXPECT_EQ(result[3].first, 4);
    EXPECT_EQ(result[3].second, 5);
}

TEST(RppPhase5Pipeline, TakeWhileDropWhile) {
    auto v = std::vector{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto result = v
        | rpp::drop_while([](int x) { return x <= 3; })
        | rpp::take_while([](int x) { return x <= 7; })
        | rpp::to_vector;
    EXPECT_EQ(result, (std::vector{4, 5, 6, 7}));
}

// ============================================================================
// Edge cases
// ============================================================================

TEST(RppEdge, EmptyRangeAllOps) {
    auto v = std::vector<int>{};
    EXPECT_TRUE((v | rpp::filter([](int) { return true; }) | rpp::to_vector).empty());
    EXPECT_TRUE((v | rpp::sort() | rpp::to_vector).empty());
    EXPECT_TRUE((v | rpp::distinct() | rpp::to_vector).empty());
    EXPECT_EQ(v | rpp::sum(), 0);
    EXPECT_EQ(v | rpp::count, 0);
    EXPECT_FALSE((v | rpp::first).has_value());
    EXPECT_FALSE((v | rpp::last).has_value());
}

TEST(RppEdge, SingleElement) {
    auto v = std::vector{42};
    EXPECT_EQ((v | rpp::to_vector), (std::vector{42}));
    EXPECT_EQ(v | rpp::sum(), 42);
    EXPECT_EQ(*( v | rpp::first ), 42);
    EXPECT_EQ(*( v | rpp::last ), 42);
    auto sorted = v | rpp::sort() | rpp::to_vector;
    EXPECT_EQ(sorted, (std::vector{42}));
}

TEST(RppEdge, LargeRange) {
    std::vector<int> v(10000);
    std::iota(v.begin(), v.end(), 0);
    
    auto result = v 
        | rpp::filter([](int x) { return x % 100 == 0; })
        | rpp::sort()
        | rpp::to_vector;
    EXPECT_EQ(result.size(), 100u);
    EXPECT_EQ(result[0], 0);
    EXPECT_EQ(result[99], 9900);
}
