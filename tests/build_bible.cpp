// clang++ tests/build_bible.cpp -std=c++2a -O3 -o bible_test
// rm bible-index*; ./bible_test

#include <cstdint>
#include <deque>
#include <iostream>
#include <string>
#include <fstream>
#include <deque>
#include <unordered_map>
#include <unordered_set>

#include "../src/common/InvertedIndex.h"
#include "../src/common/utils.h"
#include "../src/uint64/UInt64Row.h"

using namespace cpot;

int main() {
  std::unordered_map<std::string, uint64_t> tokenMap;

  std::vector<UInt64Row> gt;
  std::unordered_map<std::string, uint64_t> counts;
  {
    auto t0 = std::chrono::high_resolution_clock::now();
    std::ifstream testFile("bible.txt");
    std::string line;

    InvertedIndex<UInt64Row> index("bible-index");

    uint64_t lineNum = 1;

    while(getline(testFile, line)) {
      std::deque<std::string> words = split(line, ' ');
      std::unordered_set<std::string> seen;
      for (auto word : words) {
        if (seen.contains(word)) {
          continue;
        }
        if (word == "the") {
          gt.push_back(UInt64Row{lineNum});
        }
        seen.insert(word);
        if (!tokenMap.contains(word)) {
          tokenMap.insert(std::make_pair(std::string(word), uint64_t(tokenMap.size() + 1)));
          counts.insert(std::make_pair(std::string(word), 1));
        } else {
          counts.at(std::string(word)) += 1;
        }
        uint64_t token = tokenMap.at(word);
        index.insert(token, UInt64Row{lineNum});
      }
      ++lineNum;
    }

    index.flush();

    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << "ms" << std::endl;
  }

  {
    auto t0 = std::chrono::high_resolution_clock::now();
    InvertedIndex<UInt64Row> index("bible-index");
    auto all = index.all(tokenMap.at("the"));
    if (all != gt) {
      throw std::runtime_error("bad search results");
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << all.size() << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << "ms" << std::endl;

    {
      auto all = index.all(tokenMap.at("honey"));
      std::cout << all.size() << " " << counts.at("honey") << std::endl;
    }
  }
}
