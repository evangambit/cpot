#include "gtest/gtest.h"

#include <algorithm>
#include <set>

#include "../src/common/Iterator.h"
#include "../src/uint64/UInt64Row.h"

using namespace cpot;

typedef IteratorInterface<UInt64Row> Iterator;

namespace {

template<class T>
std::shared_ptr<VectorIterator<T>> make_iterator(const std::set<T>& A) {
  std::vector<T> vec(A.begin(), A.end());
  return std::make_shared<VectorIterator<UInt64Row>>(vec);
}

TEST(IntersectionTests, VectorIterator) {
  auto it = std::make_shared<VectorIterator<UInt64Row>>(std::vector<UInt64Row>({
    UInt64Row{1},
    UInt64Row{2},
    UInt64Row{3},
  }));
  EXPECT_NE(it, nullptr);
  ASSERT_EQ(it->currentValue, UInt64Row{1});
  ASSERT_EQ(it->next(), UInt64Row{2});
  ASSERT_EQ(it->next(), UInt64Row{3});
  ASSERT_EQ(it->next(), UInt64Row::largest());
}

TEST(IntersectionTests, IntersectIdentical) {
  auto it1 = std::make_shared<VectorIterator<UInt64Row>>(std::vector<UInt64Row>({
    UInt64Row{1},
    UInt64Row{2},
    UInt64Row{3},
  }));
  auto it2 = std::make_shared<VectorIterator<UInt64Row>>(std::vector<UInt64Row>({
    UInt64Row{1},
    UInt64Row{2},
    UInt64Row{3},
  }));
  std::vector<std::shared_ptr<Iterator>> iters({it1, it2});
  auto it = std::make_shared<IntersectionIterator<UInt64Row>>(iters);
  EXPECT_NE(it, nullptr);
  ASSERT_EQ(it->currentValue, UInt64Row{1});
  ASSERT_EQ(it->next(), UInt64Row{2});
  ASSERT_EQ(it->next(), UInt64Row{3});
  ASSERT_EQ(it->next(), UInt64Row::largest());
}

TEST(IntersectionTests, IntersectRandom) {
  int numIdenticalTests = 0;
  int numDifferentTests = 0;
  int numEmptyTests = 0;

  for (size_t i = 0; i < 10'000; ++i) {
    std::set<UInt64Row> A;
    std::set<UInt64Row> B;
    for (size_t j = 0; j < rand() % 6; ++j) {
      A.insert(UInt64Row{uint64_t(rand() % 10)});
      B.insert(UInt64Row{uint64_t(rand() % 10)});
    }
    std::vector<std::shared_ptr<Iterator>> iters = {
      make_iterator(A),
      make_iterator(B),
    };

    std::vector<UInt64Row> groundTruth;
    std::set_intersection(A.begin(), A.end(), B.begin(), B.end(), std::back_inserter(groundTruth));

    if (groundTruth.size() == A.size() && groundTruth.size() == B.size()) {
      ++numIdenticalTests;
    } else {
      ++numDifferentTests;
    }
    if (A.size() == 0 || B.size() == 0) {
      ++numEmptyTests;
    }

    auto it = std::make_shared<IntersectionIterator<UInt64Row>>(iters);
    EXPECT_NE(it, nullptr);

    if (groundTruth.size() == 0) {
      ASSERT_EQ(it->currentValue, UInt64Row::largest());
      continue;
    }
    ASSERT_EQ(it->currentValue, groundTruth[0]);
    for (size_t i = 1; i < groundTruth.size(); ++i) {
      ASSERT_EQ(it->next(), groundTruth[i]);
    }
    ASSERT_EQ(it->next(), UInt64Row::largest());
  }
  ASSERT_GT(numIdenticalTests, 5);
  ASSERT_GT(numDifferentTests, 1000);
  ASSERT_GT(numEmptyTests, 20);
}

TEST(IntersectionTests, UnionRandom) {
  int numIdenticalTests = 0;
  int numDifferentTests = 0;
  int numEmptyTests = 0;

  for (size_t i = 0; i < 10'000; ++i) {
    std::set<UInt64Row> A;
    std::set<UInt64Row> B;
    for (size_t j = 0; j < rand() % 6; ++j) {
      A.insert(UInt64Row{uint64_t(rand() % 10)});
      B.insert(UInt64Row{uint64_t(rand() % 10)});
    }
    std::vector<std::shared_ptr<Iterator>> iters = {
      make_iterator(A),
      make_iterator(B),
    };

    std::vector<UInt64Row> groundTruth;
    std::set_union(A.begin(), A.end(), B.begin(), B.end(), std::back_inserter(groundTruth));

    if (groundTruth.size() == A.size() && groundTruth.size() == B.size()) {
      ++numIdenticalTests;
    } else {
      ++numDifferentTests;
    }
    if (A.size() == 0 || B.size() == 0) {
      ++numEmptyTests;
    }

    auto it = std::make_shared<UnionIterator<UInt64Row>>(iters);
    EXPECT_NE(it, nullptr);

    if (groundTruth.size() == 0) {
      ASSERT_EQ(it->currentValue, UInt64Row::largest());
      continue;
    }
    ASSERT_EQ(it->currentValue, groundTruth[0]);
    for (size_t i = 1; i < groundTruth.size(); ++i) {
      ASSERT_EQ(it->next(), groundTruth[i]);
    }
    ASSERT_EQ(it->next(), UInt64Row::largest());
  }
  ASSERT_GT(numIdenticalTests, 5);
  ASSERT_GT(numDifferentTests, 1000);
  ASSERT_GT(numEmptyTests, 20);
}

}  // namespace

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}

