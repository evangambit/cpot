// clang++ tests/memory_page_manager_tests.cpp -I/opt/homebrew/Cellar/googletest/1.14.0/include -std=c++20 -L/opt/homebrew/Cellar/googletest/1.14.0/lib -lgtest

#include "gtest/gtest.h"

#include <set>

#include "../src/common/MemoryPageManager.h"

using namespace cpot;

namespace {

TEST(MemoryPageManagerTests, Allocation) {
  auto manager = std::make_shared<MemoryPageManager<uint32_t>>();
  PageLoc loc1;
  uint32_t *a = manager->new_page(&loc1);

  PageLoc loc2;
  uint32_t *b = manager->new_page(&loc2);

  ASSERT_EQ(loc1, 0);
  ASSERT_EQ(loc2, 1);
  ASSERT_EQ(loc1 + 1, loc2);
}

TEST(MemoryPageManagerTests, Loading) {
  auto manager = std::make_shared<MemoryPageManager<uint32_t>>();
  PageLoc loc;
  uint32_t *a = manager->new_page(&loc);
  uint32_t const *b = manager->load_page(loc);
  ASSERT_EQ(a, b);
}

TEST(MemoryPageManagerTests, MemoryUsage) {
  auto manager = std::make_shared<MemoryPageManager<uint32_t>>();
  PageLoc loc;
  manager->new_page(&loc);
  ASSERT_EQ(manager->currentMemoryUsed(), sizeof(uint32_t) * 1);
  manager->new_page(&loc);
  ASSERT_EQ(manager->currentMemoryUsed(), sizeof(uint32_t) * 2);
}

TEST(MemoryPageManagerTests, Isempty) {
  auto manager = std::make_shared<MemoryPageManager<uint32_t>>();
  ASSERT_TRUE(manager->empty());
  PageLoc loc;
  manager->new_page(&loc);
  ASSERT_FALSE(manager->empty());
}



}  // namespace

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}

