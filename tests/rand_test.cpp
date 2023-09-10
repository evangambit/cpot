// clang++ tests/rand_test.cpp -std=c++2a -o rand_test
// rm -rf rand-test-index*; ./rand_test

#include <cstdint>
#include <deque>
#include <iostream>
#include <set>
#include <string>
#include <random>
#include <fstream>
#include <vector>

#include "../src/common/InvertedIndex.h"
#include "../src/common/utils.h"
#include "../src/common/DiskPageManager.h"
#include "../src/common/SkipTree.h"
#include "../src/uint64/UInt64Row.h"

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

template<class T>
void assert_eq(SkipTree<T>& tree, std::set<T>& set) {
  std::vector<T> A = tree.all();
  std::vector<T> B(set.begin(), set.end());
  if (A.size() != B.size()) {
    std::cout << A.size() << " !=  " << B.size() << std::endl;
    assert(false);
  }
  for (size_t i = 0; i < A.size(); ++i) {
    if (A[i] != B[i]) {
      std::cout << A[i] << " != " << B[i] << std::endl;
      assert(false);
    }
  }
}

int main() {
  constexpr uint64_t token = 1;
  constexpr uint64_t high = 100'000;

  // auto pageManager = std::make_shared<DiskPageManager<SkipTree<UInt64Row>::Node>>("rand-test-index");
  // SkipTree<UInt64Row> tree(pageManager, -1);
  // for (uint64_t i = 1; i <= high; ++i) {
  //   UInt64Row v = UInt64Row{i};
  //   tree.insert(v);
  // }

  // std::set<UInt64Row> groundtruth;
  // {
  //   auto pageManager = std::make_shared<DiskPageManager<SkipTree<UInt64Row>::Node>>("rand-test-index");
  //   SkipTree<UInt64Row> tree(pageManager, kNullPage);
  //   for (size_t i = 0; i < high; ++i) {
  //     UInt64Row v = UInt64Row{uint64_t(rand() % high + 1)};
  //     groundtruth.insert(v);
  //     tree.insert(v);
  //   }
  //   assert(tree.rootLoc_ == 0);
  //   assert_eq(tree, groundtruth);

  //   for (size_t i = 0; i < high; ++i) {
  //     UInt64Row v = UInt64Row{uint64_t(rand() % high + 1)};
  //     groundtruth.erase(v);
  //     tree.remove(v);
  //   }
  //   assert_eq(tree, groundtruth);
  // }






  {
    InvertedIndex<UInt64Row> index("rand-test-index");

    std::vector<uint64_t> A;
    for (uint64_t i = 1; i <= high; ++i) {
      A.push_back(i);
    }
    shuffle(A.begin(), A.end());

    for (auto it : A) {
      index.insert(token, UInt64Row{it});
    }
  }

  {
    InvertedIndex<UInt64Row> index("rand-test-index");

    if (index.count(token) != high) {
      throw std::runtime_error("token count incorrect " + std::to_string(index.count(token)));
    }

    std::shared_ptr<IteratorInterface<UInt64Row>> it = index.iterator(token);

    std::vector<UInt64Row> R = iter2vec(it);
    if (R.size() != high) {
      throw std::runtime_error("R.size() != high (" + std::to_string(R.size()) + " vs " + std::to_string(high) + ")");
    }
    for (uint64_t i = 0; i < R.size(); ++i) {
      if (R[i].val != i + 1) {
        throw std::runtime_error("R[i] != i + 1");
      }
    }

    std::cout << "passed" << std::endl;
  }

  return 0;
}
