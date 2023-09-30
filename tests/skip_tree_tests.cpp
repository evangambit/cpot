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

std::vector<UInt64Row> range(uint64_t low, uint64_t high) {
  std::vector<UInt64Row> r;
  for (uint64_t i = low; i < high; ++i) {
    r.push_back(UInt64Row{i});
  }
  return r;
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

TEST(SkipTreeTest, ValuesAreOrdered) {
  std::vector<UInt64Row> A = range(0, 100'000);
  shuffle(A.begin(), A.end());
  auto pageManager = std::make_shared<MemoryPageManager<SkipTree<UInt64Row>::Node>>();
  SkipTree<UInt64Row> skipTree(pageManager, kNullPage);
  for (auto a : A) {
    skipTree.insert(a);
  }
  EXPECT_EQ(skipTree.all(), range(0, 100'000));
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

TEST(SkipTreeTest, Range) {
  const uint64_t high = 1000;
  auto pageManager = std::make_shared<MemoryPageManager<SkipTree<UInt64Row>::Node>>();
  SkipTree<UInt64Row> tree(pageManager, kNullPage);

  for (uint64_t i = 1; i <= high; ++i) {
    tree.insert(UInt64Row{i});
  }

  std::vector<UInt64Row> A = tree.range(100, 150);

  ASSERT_EQ(A, range(100, 150));
}

TEST(SkipTreeTest, EmptyTree) {
  const uint64_t high = 100;
  std::vector<UInt64Row> A = range(0, high);
  shuffle(A.begin(), A.end());
  auto pageManager = std::make_shared<MemoryPageManager<SkipTree<UInt64Row>::Node>>();
  SkipTree<UInt64Row> tree(pageManager, kNullPage);
  for (auto a : A) {
    tree.insert(a);
  }
  for (auto a : A) {
    ASSERT_TRUE(tree.remove(a));
  }
  ASSERT_EQ(tree.all(), std::vector<UInt64Row>());
}


}  // namespace

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}

