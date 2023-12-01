#ifndef UInt32PairRow_H
#define UInt32PairRow_H

#include <cstdint>
#include <iostream>
#include <stdexcept>

using namespace cpot;

struct UInt32PairRow {
  uint32_t docid;
  uint32_t value;
  static UInt32PairRow smallest() {
    return UInt32PairRow{0, 0};
  }
  static UInt32PairRow largest() {
    return UInt32PairRow{uint32_t(-1), uint32_t(-1)};
  }
  static UInt32PairRow make(uint32_t docid, uint32_t value) {
    UInt32PairRow rtn = UInt32PairRow{};
    rtn.docid = docid;
    rtn.value = value;
    return rtn;
  }
  bool operator<(UInt32PairRow that) const {
    if (this->docid != that.docid) {
      return this->docid < that.docid;
    }
    return this->value < that.value;
  }
  bool operator==(UInt32PairRow that) const {
    return docid == that.docid && this->value == that.value;
  }
  bool operator<=(UInt32PairRow that) const {
    if (this->docid != that.docid) {
      return this->docid <= that.docid;
    }
    return this->value <= that.value;
  }
  UInt32PairRow next() const {
    return UInt32PairRow{docid + 1};
  }
  UInt32PairRow prev() const {
    return UInt32PairRow{docid - 1};
  }
};

std::ostream& operator<<(std::ostream& s, const UInt32PairRow& row) {
  return s << "(" << row.docid << ":" << row.value << ")";
}

#endif  // UInt32PairRow
