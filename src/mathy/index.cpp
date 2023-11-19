// clang++ src/mathy/index.cpp -std=c++2a -O3 -o index

#include <cstdint>
#include <deque>
#include <fstream>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

#include "../common/InvertedIndex.h"
#include "../common/utils.h"
#include "MathyRow.h"

using namespace cpot;

struct IntPair {
  uint32_t x;
  uint32_t y;
  static IntPair make(uint32_t x, uint32_t y) {
    IntPair rtn = IntPair{};
    rtn.x = x;
    rtn.y = y;
    return rtn;
  }
  bool operator<(IntPair that) const {
    if (this->x < that.x) {
      return true;
    } else if (this->x > that.x) {
      return false;
    } else if (this->y < that.y) {
      return true;
    } else if (this->y > that.y) {
      return false;
    }
    return false;
  }
  bool operator==(IntPair that) const {
    return (x == that.x) && (y == that.y);
  }
  bool operator<=(IntPair that) const {
    if (this->x < that.x) {
      return true;
    } else if (this->x > that.x) {
      return false;
    } else if (this->y < that.y) {
      return true;
    } else if (this->y > that.y) {
      return false;
    }
    return true;
  }
  static IntPair from_string(std::string s) {
    std::deque<std::string> parts = split(s, ':');
    if (parts.size() != 2) {
      throw std::invalid_argument("Invalid row: must be 123:456");
    }
    uint32_t docid = static_cast<uint32_t>(std::stoul(parts[0]));
    uint32_t value = static_cast<uint32_t>(std::stoul(parts[1]));
    return IntPair::make(docid, value);
  }
};

std::ostream& operator<<(std::ostream& s, const IntPair& row) {
  return s << row.x << ":" << row.y;
}


struct InvertedIndexCLI {
  InvertedIndexCLI(std::string path) : db(path) {}
  void run() {
    process_std_lines([this](const std::string& line) -> bool {
      if (line == "help") {
        std::cout << "Commands:" << std::endl;
        std::cout << "insert <docid> <token1>:<value1> <token2>:<value2> ..." << std::endl;
        std::cout << "print_all <token>" << std::endl;
        std::cout << "search <token1>:<weight1> <token2>:<weight2> ...    # rows which contain all tokens " << std::endl;
        std::cout << "memory_used  # memory, in bytes, used for caching" << std::endl;
        std::cout << "flush        # clears the cache" << std::endl;
        return false;
      }
      if (line == "quit") {
        this->db.flush();
        return true;
      }
      if (line == "flush") {
        db.flush();
        return false;
      }
      if (line == "commit") {
        this->db.commit();
        return false;
      }
      if (line == "memory_used") {
        std::cout << db.currentMemoryUsed() << std::endl;
        return false;
      }

      std::deque<std::string> parts = split(line, ' ');
      if (parts.size() == 0) {
        return false;
      }

      if (parts[0] == "print_all") {
        if (parts.size() != 2) {
          std::cout << "Unrecognized usage of \"print_all\" command" << std::endl;
          return false;
        }
        uint32_t token = std::stoull(parts[1]);

        std::vector<MathyRow> rows = db.all(token);
        for (auto it = rows.begin(); it != rows.end(); ++it) {
          std::cout << " " << *it;
        }
        std::cout << std::endl;
        return false;
      }
      if (parts[0] == "count") {
        parts.pop_front();

        std::deque<uint32_t> ints;
        for (size_t i = 0; i < parts.size(); ++i) {
          try {
            ints.push_back(std::stoull(parts[i]));
          } catch (std::invalid_argument&) {
            std::cout << "Unrecognized integer" << std::endl;
            return false;
          }
        }

        for (auto it = ints.begin(); it != ints.end(); ++it) {
          std::cout << *it << " " << db.count(*it) << std::endl;
        }
        
        return false;
      }
      if (parts[0] == "insert") {
        parts.pop_front();
        try {
          uint32_t docid = static_cast<uint32_t>(std::stoul(parts[0]));
          parts.pop_front();
          for (size_t i = 0; i < parts.size(); ++i) {
            uint32_t tokenid = static_cast<uint32_t>(std::stoul(parts[0]));
            uint32_t value = static_cast<uint32_t>(std::stoul(parts[0]));
            std::deque<std::string> x = split(parts[i], ':');
            if (x.size() != 2) {
              std::cout << "Unrecognized row" << std::endl;
              return false;
            }
            MathyRow row = MathyRow::make(docid, value);
            db.insert(tokenid, row);
          }
          return false;
        } catch (std::invalid_argument&) {
          std::cout << "Unrecognized row" << std::endl;
          return false;
        }
      }
      if (parts[0] == "remove") {
        std::cout << "Not yet implemented." << std::endl;
        return false;
      }
      if (parts[0] == "search") {
        parts.pop_front();
        this->handle_search_command(parts);
        return false;
      }
      std::cout << "Unrecognized command" << std::endl;
      return false;
    });
  }
  std::pair<bool, std::deque<IntPair>> _strings_to_intpairs(const std::deque<std::string>& A) {
    std::deque<IntPair> r;
    for (size_t i = 0; i < A.size(); ++i) {
      try {
        r.push_back(IntPair::from_string(A[i]));
      } catch (std::invalid_argument&) {
        return std::make_pair(false, r);
      }
    }
    return std::make_pair(true, r);
  }
  void handle_search_command(const std::deque<std::string> command) {
    auto parsed = this->_strings_to_intpairs(command);
    if (!parsed.first) {
      std::cout << "Failed to parse integer" << std::endl;
      return;
    }

    uint64_t *results = new uint64_t[1000000];
    for (auto row : parsed.second) {
      uint32_t tokenid = row.x;
      uint32_t weight = row.y;
      std::vector<MathyRow> rows = db.all(tokenid);
      for (auto row : rows) {
        if (row.docid >= 1000000) {
          std::cout << "More than 1 million documents" << std::endl;
          delete[] results;
          return;
        }
        results[row.docid] += weight * row.value;
      }
    }

    std::priority_queue<IntPair> pq;
    for (int i = 0; i < 1000000; ++i) {
      if (results[i] <= 0) continue;
      pq.push(IntPair::make(results[i], i));
      if (pq.size() > 100) {
        pq.pop();
      }
    }

    while (pq.size() > 0) {
      IntPair ip = pq.top();
      pq.pop();
      std::cout << ip.y << ":" << ip.x << std::endl;
    }
    delete[] results;
  }
  InvertedIndex<MathyRow> db;
};

int main() {
  InvertedIndexCLI cli("test-index/index");
  cli.run();
}
