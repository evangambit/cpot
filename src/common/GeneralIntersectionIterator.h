#ifndef GENERAL_INTERSECTION_ITERATOR_H
#define GENERAL_INTERSECTION_ITERATOR_H

#include "Iterator.h"

namespace cpot {

/**
 * A more sophisticated version of IntersectionIterator that can handle negated iterators.
 */

template<class Row>
struct GeneralIntersectionIterator : public IteratorInterface<Row> {
  GeneralIntersectionIterator(std::vector<std::pair<std::shared_ptr<IteratorInterface<Row>>, bool>>& iters) : iters_(iters) {
    if (iters_.size() == 0) {
      throw std::runtime_error("GeneralIntersectionIterator requires at least one iterator");
    }
    numNonNegatedIterators_ = 0;
    for (auto it : iters_) {
      numNonNegatedIterators_ += !it.second;
    }
    if (numNonNegatedIterators_ == 0) {
      throw std::runtime_error("GeneralIntersectionIterator requires at least one non-negated iterator");
    }
    this->skip_to(this->_max_non_negated().first);
  }
  // Returns the value of the largest (non-negated) iterator, as well as the
  // number of non-negated iterators with that value.
  std::pair<Row, size_t> _max_non_negated() {
    Row r = Row::smallest();
    size_t numWithMax = 0;
    for (auto iter : iters_) {
      if (!iter.second) {
        if (r < iter.first->currentValue) {
          r = iter.first->currentValue;
          numWithMax = 1;
        } else if (r == iter.first->currentValue) {
          ++numWithMax;
        }
      }
    }
    return std::make_pair(r, numWithMax);
  }
  Row skip_to(Row row) override {
    for (auto iter : iters_) {
      iter.first->skip_to(row);
    }
    std::pair<Row, size_t> x = this->_max_non_negated();

    while (true) {
      if (x.first == Row::largest()) {
        return this->currentValue = x.first;
      }
      if (x.second == numNonNegatedIterators_) {
        bool isMatch = true;
        for (auto iter : iters_) {
          if (iter.second && iter.first->skip_to(x.first) == x.first) {
            isMatch = false;
            break;
          }
        }
        if (isMatch) {
          return this->currentValue = x.first;
        } else {
          for (auto iter : iters_) {
            iter.first->next();
          }
        }
      } else {
        for (auto iter : iters_) {
          iter.first->skip_to(x.first);
        }
      }
      x = this->_max_non_negated();
    }
  }
  Row next() override {
    // TODO: make this more efficient.
    this->skip_to(this->currentValue.next());
    return this->currentValue;
  }
  std::vector<std::pair<std::shared_ptr<IteratorInterface<Row>>, bool>>& iters_;
  size_t numNonNegatedIterators_;
};

}  // namespace cpot

#endif  // GENERAL_INTERSECTION_ITERATOR_H