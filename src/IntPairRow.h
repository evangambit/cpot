#ifndef IntPairRow_H
#define IntPairRow_H

#include <cstdint>
#include <iostream>
#include <stdexcept>

using namespace cpot;

struct IntPairRow {
  uint64_t docid;
  uint64_t value;
  static IntPairRow smallest() {
    return IntPairRow{0};
  }
  static IntPairRow largest() {
    return IntPairRow{uint32_t(-1)};
  }
  static IntPairRow make(uint32_t docid, uint32_t value) {
    IntPairRow rtn = IntPairRow{};
    rtn.docid = docid;
    rtn.value = value;
    return rtn;
  }
  bool operator<(IntPairRow that) const {
    if (this->docid != that.docid) {
      return this->docid < that.docid;
    }
    return this->value < that.value;
  }
  bool operator==(IntPairRow that) const {
    return docid == that.docid && this->value == that.value;
  }
  bool operator<=(IntPairRow that) const {
    if (this->docid != that.docid) {
      return this->docid <= that.docid;
    }
    return this->value <= that.value;
  }
  IntPairRow next() const {
    return IntPairRow{docid + 1};
  }
  IntPairRow prev() const {
    return IntPairRow{docid - 1};
  }
};

std::ostream& operator<<(std::ostream& s, const IntPairRow& row) {
  return s << "(" << row.docid << ":" << row.value << ")";
}

#endif  // IntPairRow
