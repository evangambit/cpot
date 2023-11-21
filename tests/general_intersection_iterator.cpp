#include "gtest/gtest.h"

#include <algorithm>
#include <set>

#include "../src/common/Iterator.h"
#include "../src/common/utils.h"
#include "../src/common/GeneralIntersectionIterator.h"
#include "../src/uint64/UInt64Row.h"

using namespace cpot;

namespace {

template<class T>
std::shared_ptr<VectorIterator<T>> make_iterator(const std::set<T>& A) {
  std::vector<T> vec(A.begin(), A.end());
  return std::make_shared<VectorIterator<UInt64Row>>(vec);
}


TEST(IntersectionTests, IntersectRandom) {
  for (size_t i = 0; i < 10'000; ++i) {
    std::set<UInt64Row> A;
    std::set<UInt64Row> B;
    std::set<UInt64Row> C;
    for (size_t j = 0; j < rand() % 6; ++j) {
      A.insert(UInt64Row{uint64_t(rand() % 10)});
      B.insert(UInt64Row{uint64_t(rand() % 10)});
      C.insert(UInt64Row{uint64_t(rand() % 10)});
    }
    std::vector<std::shared_ptr<NegatableIterator<UInt64Row>>> iters = {
      std::make_shared<NegatableIterator<UInt64Row>>(make_iterator(A), false),
      std::make_shared<NegatableIterator<UInt64Row>>(make_iterator(B), false),
      std::make_shared<NegatableIterator<UInt64Row>>(make_iterator(C), true),
    };

    std::vector<UInt64Row> aAndB;
    std::set_intersection(A.begin(), A.end(), B.begin(), B.end(), std::back_inserter(aAndB));

    std::vector<UInt64Row> groundTruth;
    for (auto x : aAndB) {
      if (C.find(x) == C.end()) {
        groundTruth.push_back(x);
      }
    }

    auto it = std::make_shared<GeneralIntersectionIterator<UInt64Row>>(iters);
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
}

}  // namespace

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}

