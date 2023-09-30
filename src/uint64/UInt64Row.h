#ifndef U_Int_64_Row_H
#define U_Int_64_Row_H

#include <cstdint>
#include <iostream>

struct UInt64Row {
  uint64_t val;
  UInt64Row() {}
  UInt64Row(uint64_t val) : val(val) {}
  static UInt64Row smallest() {
    return UInt64Row{0};
  }
  static UInt64Row largest() {
    return UInt64Row{uint64_t(-1)};
  }
  static UInt64Row from_string(std::string t) {
    return UInt64Row{std::stoull(t)};
  }
  std::string to_string() const {
    return std::to_string(val);
  }
  bool operator<(UInt64Row that) const {
    return this->val < that.val;
  }
  bool operator==(UInt64Row that) const {
    return val == that.val;
  }
  bool operator<=(UInt64Row that) const {
    return this->val <= that.val;
  }
  UInt64Row next() const {
    return UInt64Row{val + 1};
  }
  UInt64Row prev() const {
    return UInt64Row{val - 1};
  }
};

std::ostream& operator<<(std::ostream& s, const UInt64Row& row) {
  return s << row.val;
}

#endif  // U_Int_64_Row_H
