#include <gtest/gtest.h>
#include "frappe/version.hpp"

TEST(VersionTest, MajorMinorPatch) {
    EXPECT_GE(frappe::version::major, 0u);
    EXPECT_GE(frappe::version::minor, 0u);
    EXPECT_GE(frappe::version::patch, 0u);
}

TEST(VersionTest, Number) {
    auto num = frappe::version::number();
    EXPECT_EQ(num, frappe::version::major * 10000 + 
                   frappe::version::minor * 100 + 
                   frappe::version::patch);
}

TEST(VersionTest, String) {
    auto str = frappe::version::string();
    EXPECT_FALSE(str.empty());
    EXPECT_NE(str.find('.'), std::string_view::npos);
}
