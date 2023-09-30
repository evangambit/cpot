// clang++ tests/skip_tree_tests.cpp -I/opt/homebrew/Cellar/googletest/1.14.0/include -std=c++20 -L/opt/homebrew/Cellar/googletest/1.14.0/lib -lgtest

#include "gtest/gtest.h"

#include <set>

#include "../src/common/SkipTree.h"
#include "../src/common/MemoryPageManager.h"
#include "../src/uint64/UInt64Row.h"

using namespace cpot;

namespace {

template<class T>
std::ostream& operator<<(std::ostream& s, const std::vector<T>& A) {
  if (A.size() == 0) {
    return s << "{}";
  }
  s << "{" << A[0];
  for (size_t i = 1; i < A.size(); ++i) {
    s << ", " << A[i];
  }
  return s << "}";
}

// Copied from https://en.cppreference.com/w/cpp/algorithm/random_shuffle
template<class RandomIt>
void shuffle(RandomIt first, RandomIt last) {
  typedef typename std::iterator_traits<RandomIt>::difference_type diff_t;
  for (diff_t i = last - first - 1; i > 0; --i) {
    std::swap(first[i], first[std::rand() % (i + 1)]);
  }
}

template<class Row>
std::vector<Row> iter2vec(std::shared_ptr<IteratorInterface<Row>> it) {
  std::vector<Row> r;
  const Row big = Row::largest();
  while (it->currentValue < big) {
    r.push_back(it->currentValue);
    it->next();
  }
  return r;
}

TEST(SkipTreeTest, InsertIntoEmptyTree) {
  auto pageManager = std::make_shared<MemoryPageManager<SkipTree<UInt64Row>::Node>>();
  SkipTree<UInt64Row> skipTree(pageManager, kNullPage);
  skipTree.insert(10);
  std::vector<UInt64Row> gt = {10};
  EXPECT_EQ(skipTree.all(), gt);
}

TEST(SkipTreeTest, ValuesAreOrdered) {
  auto pageManager = std::make_shared<MemoryPageManager<SkipTree<UInt64Row>::Node>>();
  SkipTree<UInt64Row> skipTree(pageManager, kNullPage);
  skipTree.insert(4);
  skipTree.insert(2);
  skipTree.insert(5);
  skipTree.insert(3);
  skipTree.insert(1);
  std::vector<UInt64Row> gt = {1, 2, 3, 4, 5};
  EXPECT_EQ(skipTree.all(), gt);
}

TEST(SkipTreeTest, RandomDelete) {
  const uint64_t high = 100'000;
  auto pageManager = std::make_shared<MemoryPageManager<SkipTree<UInt64Row>::Node>>();

  SkipTree<UInt64Row> tree(pageManager, kNullPage);
  std::set<uint64_t> gt;

  std::vector<uint64_t> vec;
  for (uint64_t i = 1; i <= high; ++i) {
    gt.insert(i);
    vec.push_back(i);
    tree.insert(i);
  }
  shuffle(vec.begin(), vec.end());

  ASSERT_EQ(tree.all(), std::vector<UInt64Row>(gt.begin(), gt.end()));

  for (size_t i = 1; i <= high / 2; ++i) {
    UInt64Row v{vec[i]};
    gt.erase(gt.find(vec[i]));
    ASSERT_TRUE(tree.remove(v));
  }

  ASSERT_EQ(tree.all(), std::vector<UInt64Row>(gt.begin(), gt.end()));
}


}  // namespace

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}

