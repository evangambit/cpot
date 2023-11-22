#ifndef MathyRow_H
#define MathyRow_H

#include <cstdint>
#include <iostream>
#include <stdexcept>

using namespace cpot;

struct MathyRow {
  uint64_t docid;
  uint64_t value;
  static MathyRow smallest() {
    return MathyRow{0};
  }
  static MathyRow largest() {
    return MathyRow{uint32_t(-1)};
  }
  static MathyRow make(uint32_t docid, uint32_t value) {
    MathyRow rtn = MathyRow{};
    rtn.docid = docid;
    rtn.value = value;
    return rtn;
  }
  // static MathyRow from_string(std::string s) {
  //   std::deque<std::string> parts = split(s, ':');
  //   if (parts.size() != 2) {
  //     throw std::invalid_argument("Invalid row: must be 123:456");
  //   }
  //   uint32_t docid = static_cast<uint32_t>(std::stoul(parts[0]));
  //   uint32_t value = static_cast<uint32_t>(std::stoul(parts[1]));
  //   return MathyRow::make(docid, value);
  // }
  // std::string to_string() const {
  //   return "(" + std::to_string(docid) + ":" + std::to_string(value) + ")";
  // }
  bool operator<(MathyRow that) const {
    if (this->docid != that.docid) {
      return this->docid < that.docid;
    }
    return this->value < that.value;
  }
  bool operator==(MathyRow that) const {
    return docid == that.docid && this->value == that.value;
  }
  bool operator<=(MathyRow that) const {
    if (this->docid != that.docid) {
      return this->docid <= that.docid;
    }
    return this->value <= that.value;
  }
  MathyRow next() const {
    return MathyRow{docid + 1};
  }
  MathyRow prev() const {
    return MathyRow{docid - 1};
  }
};

std::ostream& operator<<(std::ostream& s, const MathyRow& row) {
  return s << "(" << row.docid << ":" << row.value << ")";
}

#endif  // MathyRow
