// clang++ tests/tmp.cpp -I/opt/homebrew/Cellar/googletest/1.14.0/include -std=c++20 -L/opt/homebrew/Cellar/googletest/1.14.0/lib -lgtest

#include "gtest/gtest.h"

namespace {

TEST(FactorialTest, Negative) {
  EXPECT_EQ(1, 1);
}

}  // namespace

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}

