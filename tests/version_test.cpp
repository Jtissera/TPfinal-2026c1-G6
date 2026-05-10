#include "gtest/gtest.h"

#include "common/version.h"

namespace {

TEST(VersionTest, StringFormat) {
    std::string v = argentum::version_string();
    EXPECT_EQ(std::count(v.begin(), v.end(), '.'), 2);
}

TEST(VersionTest, MajorIsNonNegative) {
    EXPECT_GE(argentum::VERSION_MAJOR, 0);
}

TEST(VersionTest, MinorIsNonNegative) {
    EXPECT_GE(argentum::VERSION_MINOR, 0);
}

}
