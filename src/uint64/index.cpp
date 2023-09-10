// clang++ inverted-index/uint64_index.cpp -std=c++2a -O3 -o index

#include <cstdint>
#include <deque>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#include "../common/InvertedIndex.h"
#include "../common/utils.h"
#include "UInt64Row.h"

template<class Row>
struct InvertedIndexCLI {
  InvertedIndexCLI(std::string path) : db(path) {}
  void run() {
    process_std_lines([this](const std::string& line) -> bool {
      if (line == "help") {
        std::cout << "Commands:" << std::endl;
        std::cout << "insert <docid> <token1> <token2> ..." << std::endl;
        std::cout << "print_all <token>" << std::endl;
        std::cout << "intersect <token1> <token2> ...    # rows which contain all tokens " << std::endl;
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
        uint64_t token = std::stoull(parts[1]);

        std::vector<Row> rows = db.all(token);
        for (auto it = rows.begin(); it != rows.end(); ++it) {
          std::cout << " " << *it;
        }
        std::cout << std::endl;
        return false;
      }
      if (parts[0] == "count") {
        parts.pop_front();

        std::deque<uint64_t> ints;
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

        Row row;
        try {
          row = Row::from_string(parts[0]);
        } catch (std::invalid_argument&) {
          std::cout << "Unrecognized row" << std::endl;
          return false;
        }
        parts.pop_front();

        std::deque<uint64_t> ints;
        for (size_t i = 0; i < parts.size(); ++i) {
          try {
            ints.push_back(std::stoull(parts[i]));
          } catch (std::invalid_argument&) {
            std::cout << "Unrecognized integer" << std::endl;
            return false;
          }
        }
        if (ints.size() < 1) {
          std::cout << "Insert command needs at least one token" << std::endl;
          return false;
        }
        for (auto it = ints.begin(); it != ints.end(); ++it) {
          db.insert(*it, row);
        }
        return false;
      }
      if (parts[0] == "remove") {
        parts.pop_front();

        Row row;
        try {
          row = Row::from_string(parts[0]);
        } catch (std::invalid_argument&) {
          std::cout << "Unrecognized row" << std::endl;
          return false;
        }
        parts.pop_front();

        std::deque<uint64_t> ints;
        for (size_t i = 0; i < parts.size(); ++i) {
          try {
            ints.push_back(std::stoull(parts[i]));
          } catch (std::invalid_argument&) {
            std::cout << "Unrecognized integer" << std::endl;
            return false;
          }
        }
        if (ints.size() < 1) {
          std::cout << "Insert command needs at least one token" << std::endl;
          return false;
        }
        for (auto it = ints.begin(); it != ints.end(); ++it) {
          db.remove(*it, row);
        }
        return false;
      }
      if (parts[0] == "intersect") {
        parts.pop_front();
        this->handle_intersect_command(parts);
        return false;
      }
      if (parts[0] == "union") {
        parts.pop_front();
        this->handle_union_command(parts);
        return false;
      }
      std::cout << "Unrecognized command" << std::endl;
      return false;
    });
  }
  std::pair<bool, std::deque<uint64_t>> _strings_to_uint64s(const std::deque<std::string>& A) {
    std::deque<uint64_t> r;
    for (size_t i = 0; i < A.size(); ++i) {
      try {
        r.push_back(std::stoull(A[i]));
      } catch (std::invalid_argument&) {
        return std::make_pair(false, r);
      }
    }
    return std::make_pair(true, r);
  }
  void handle_intersect_command(const std::deque<std::string> command) {
    auto parsed = this->_strings_to_uint64s(command);
    if (!parsed.first) {
      std::cout << "Failed to parse integer" << std::endl;
      return;
    }

    std::vector<std::shared_ptr<IteratorInterface<Row>>> iterators;
    for (auto token : parsed.second) {
      iterators.push_back(db.iterator(token));
    }

    std::shared_ptr<IteratorInterface<Row>> it = std::make_shared<IntersectionIterator<Row>>(iterators);
    print_iterator(it);
  }
  void handle_union_command(const std::deque<std::string> command) {
    auto parsed = this->_strings_to_uint64s(command);
    if (!parsed.first) {
      std::cout << "Failed to parse integer" << std::endl;
      return;
    }

    std::vector<std::shared_ptr<IteratorInterface<Row>>> iterators;
    for (auto token : parsed.second) {
      iterators.push_back(db.iterator(token));
    }

    std::shared_ptr<IteratorInterface<Row>> it = std::make_shared<UnionIterator<Row>>(iterators);
    print_iterator(it);
  }
  InvertedIndex<Row> db;
};

int main() {
  InvertedIndexCLI<UInt64Row> cli("uint64-test-index");
  cli.run();
}
