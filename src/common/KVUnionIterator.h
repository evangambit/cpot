#ifndef KVUnionIterator_H
#define KVUnionIterator_H

#include "Iterator.h"

namespace cpot {

/**
 * A union iterator that returns all matching values. Typically used to grab
 * all values for every document with at least one matching token.
 */

template<class Row, class Key, class Value>
struct KVUnionIterator : public IteratorInterface<std::pair<Key, std::vector<Value>>> {
  typedef std::pair<Key, std::vector<Value>> OutRow;
  KVUnionIterator(std::vector<std::shared_ptr<IteratorInterface<Row>>>& iters) : iters(iters) {
    if (iters.size() == 0) {
      throw std::runtime_error("UnionIterator requires at least one iterator");
    }
    this->currentValue.second.resize(iters.size());
    this->skip_to(std::make_pair(Row::smallest().key, std::vector<Value>()));
  }
  OutRow skip_to(OutRow row) override {
    for (size_t i = 0; i < iters.size(); ++i) {
      iters[i]->skip_to({row.first, 0});
    }
    this->_update_value();
    return this->currentValue;
  }
  OutRow next() override {
    Row low = this->__lowest();
    for (size_t i = 0; i < this->iters.size(); ++i) {
      if (this->iters[i]->currentValue == low) {
        this->iters[i]->next();
      }
    }
    this->_update_value();
    return this->currentValue;
  }
 private:
  void _update_value() {
    Row lowest = this->__lowest();
    this->currentValue.first = lowest.key;
    for (size_t i = 0; i < iters.size(); ++i) {
      if (iters[i]->currentValue == lowest) {
        this->currentValue.second[i] = iters[i]->currentValue.value;
      } else {
        this->currentValue.second[i] = Row::largest().value;
      }
    }
  }
  Row __lowest() {
    Row r = Row::largest();
    for (size_t i = 0; i < this->iters.size(); ++i) {
      if (this->iters[i]->currentValue < r) {
        r = this->iters[i]->currentValue;
      }
    }
    return r;
  }
  const std::vector<std::shared_ptr<IteratorInterface<Row>>> iters;
};

}  // namespace cpot

#endif  // KVUnionIterator_H