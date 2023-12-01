#include "gtest/gtest.h"

#include <algorithm>
#include <set>
#include <map>

#include "../src/common/Iterator.h"
#include "../src/common/KVUnionIterator.h"
#include "../src/common/utils.h"
#include "../src/UInt64KeyValueRow.h"

using namespace cpot;

namespace {

std::shared_ptr<VectorIterator<UInt64KeyValueRow>> make_iterator(const std::map<uint64_t, uint64_t>& A) {
  std::vector<UInt64KeyValueRow> vec;
  for (auto x : A) {
    vec.push_back(UInt64KeyValueRow{x.first, x.second});
  }
  return std::make_shared<VectorIterator<UInt64KeyValueRow>>(vec);
}

// bool operator<(const std::pair<uint64_t, std::vector<uint64_t>>& a, const std::pair<uint64_t, std::vector<uint64_t>>& b) {
//   return a.first < b.first;
// }

template<class K, class V>
std::ostream& operator<<(std::ostream& s, const std::map<K, V>& A) {
  s << "{" << std::endl;
  for (auto it : A) {
    s << "  " << it.first << ": " << it.second << std::endl;
  }
  return s << "}" << std::endl;
}

template<class K, class V>
std::ostream& operator<<(std::ostream& s, const std::pair<K, std::vector<V>>& pair) {
  const std::vector<V>& vec = pair.second;
  s << "(" << pair.first << ", ";
  if (vec.size() == 0) {
    s << "[]";
  } else {
    s << "[" << vec[0];
    for (size_t i = 1; i < vec.size(); ++i) {
      s << ", " << vec[i];
    }
    s << "]";
  }
  s << ")";
  return s;
}

template<class T>
std::ostream& operator<<(std::ostream& s, const std::vector<T>& vec) {
  if (vec.size() == 0) {
    return s << "[]";
  }
  s << "[" << vec[0];
  for (size_t i = 1; i < vec.size(); ++i) {
    s << ", " << vec[i];
  }
  return s << "]";
}

bool operator==(const std::pair<uint64_t, std::vector<uint64_t>>& a, const std::pair<uint64_t, std::vector<uint64_t>>& b) {
  if (a.first != b.first) {
    return false;
  }
  if (a.second.size() != b.second.size()) {
    return false;
  }
  for (size_t i = 0; i < a.second.size(); ++i) {
    if (a.second[i] != b.second[i]) {
      return false;
    }
  }
  return true;
}

TEST(KVUnionTests, Random) {
  for (size_t i = 0; i < 10'000; ++i) {
    std::map<uint64_t, uint64_t> A;
    std::map<uint64_t, uint64_t> B;
    for (size_t j = 0; j < rand() % 6; ++j) {
      A.insert(std::make_pair(uint64_t(rand() % 10), uint64_t(rand() % 10)));
      B.insert(std::make_pair(uint64_t(rand() % 10), uint64_t(rand() % 10)));
    }
    std::vector<std::shared_ptr<IteratorInterface<UInt64KeyValueRow>>> iters = {
      make_iterator(A),
      make_iterator(B),
    };

    std::vector<std::pair<uint64_t, std::vector<uint64_t>>> groundTruth;
    for (uint64_t i = 0; i < 10; ++i) {
      auto ita = A.find(i);
      auto itb = B.find(i);
      if (ita != A.end() && itb != B.end()) {
        groundTruth.push_back(std::make_pair(
          i,
          std::vector<uint64_t>({ita->second, itb->second})
        ));
      } else if (ita != A.end()) {
        groundTruth.push_back(std::make_pair(
          i,
          std::vector<uint64_t>({ita->second, UInt64KeyValueRow::largest().value})
        ));
      } else if (itb != B.end()) {
        groundTruth.push_back(std::make_pair(
          i,
          std::vector<uint64_t>({UInt64KeyValueRow::largest().value, itb->second})
        ));
      }
    }

    const auto kLastVec = std::vector<uint64_t>({uint64_t(-1), uint64_t(-1)});

    auto it = std::make_shared<KVUnionIterator<UInt64KeyValueRow, uint64_t, uint64_t>>(iters);
    EXPECT_NE(it, nullptr);

    if (groundTruth.size() == 0) {
      ASSERT_EQ(it->currentValue, std::make_pair(UInt64KeyValueRow::largest().key, kLastVec));
      continue;
    }
    ASSERT_EQ(it->currentValue, groundTruth[0]);
    for (size_t i = 1; i < groundTruth.size(); ++i) {
      ASSERT_EQ(it->next(), groundTruth[i]);
    }
    ASSERT_EQ(it->next(), std::make_pair(UInt64KeyValueRow::largest().key, kLastVec));
  }
}

}  // namespace

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}

