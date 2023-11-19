// clang++ src/nlp/main.cpp -std=c++2a -O3 -o index

#include "nlp.h"

using namespace cpot;

int main() {
  auto t0 = std::chrono::high_resolution_clock::now();
  std::ifstream testFile("bible.txt");
  std::string line;

  NlpIndex index("test-index/index");

  uint64_t lineNum = 1;

  while(getline(testFile, line)) {
    index.add_document(++lineNum, line);
  }

  index.index_.commit();

  auto t1 = std::chrono::high_resolution_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << std::endl;

  std::vector<uint64_t> docids = index.documents_with("the evening");
  std::cout << docids << std::endl;

  return 0;
}
