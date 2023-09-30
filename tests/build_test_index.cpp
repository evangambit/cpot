// clang++ build_test_index.cpp -std=c++2a -O3 -o build_test
// rm -rf test-index*; ./build_test

#include <cstdint>
#include <deque>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#include "src/common/InvertedIndex.h"
#include "src/common/utils.h"
#include "src/uint64/UInt64Row.h"

using namespace cpot;

template<class T>
void assert_eq(const T& a, const T& b) {
  if (!(a == b)) {
    std::cout << a << " != " << b << std::endl;
    assert(false);
  }
}

int main() {
  const uint64_t N = 500'000;
  {
    auto t0 = std::chrono::high_resolution_clock::now();
    InvertedIndex<UInt64Row> index("test-index");
    for (uint64_t doc = 1; doc <= N; ++doc) {
      for (uint64_t token = 1; token < 30; ++token) {
        if (doc % token == 0) {
          index.insert(token, UInt64Row{doc});
        }
      }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << std::endl;
  }

  {
    auto t0 = std::chrono::high_resolution_clock::now();
    InvertedIndex<UInt64Row> index("test-index");
    std::cout << index.all(1).size() << std::endl;
    std::cout << index.all(1).size() << std::endl;
    assert(index.all(1).size() == N);
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << std::endl;
    assert_eq(index.all(2).size(), size_t(250'000));
    assert_eq(index.all(3).size(), size_t(166'666));
    assert_eq(index.all(4).size(), size_t(125'000));
    assert_eq(index.all(5).size(), size_t(100'000));
    assert_eq(index.all(6).size(), size_t( 83'333));
  }
}
