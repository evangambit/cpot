#ifndef UInt64KeyValueRow_H
#define UInt64KeyValueRow_H

#include <cstdint>
#include <iostream>
#include <stdexcept>

using namespace cpot;

/**
 * Unlike UInt32PairRow, this struct does not consider the value field when
 * comparing two rows. This means one BTree cannot have two rows with the same
 * key.
 */
struct UInt64KeyValueRow {
  uint64_t key;
  uint64_t value;
  static UInt64KeyValueRow smallest() {
    return UInt64KeyValueRow{0, 0};
  }
  static UInt64KeyValueRow largest() {
    return UInt64KeyValueRow{uint64_t(-1), uint64_t(-1)};
  }
  static UInt64KeyValueRow make(uint64_t key, uint64_t value) {
    UInt64KeyValueRow rtn = UInt64KeyValueRow{};
    rtn.key = key;
    rtn.value = value;
    return rtn;
  }
  bool operator<(UInt64KeyValueRow that) const {
    return this->key < that.key;
  }
  bool operator==(UInt64KeyValueRow that) const {
    return key == that.key;
  }
  bool operator<=(UInt64KeyValueRow that) const {
    return this->value <= that.value;
  }
  UInt64KeyValueRow next() const {
    return UInt64KeyValueRow{key + 1};
  }
  UInt64KeyValueRow prev() const {
    return UInt64KeyValueRow{key - 1};
  }
};

std::ostream& operator<<(std::ostream& s, const UInt64KeyValueRow& row) {
  return s << "(" << row.key << ":" << row.value << ")";
}

#endif  // UInt64KeyValueRow
