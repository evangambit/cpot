#ifndef GENERAL_INTERSECTION_ITERATOR_H
#define GENERAL_INTERSECTION_ITERATOR_H

#include "Iterator.h"

namespace cpot {

template<class Row>
struct NegatableIterator : public IteratorInterface<Row> {
  NegatableIterator(std::shared_ptr<IteratorInterface<Row>> iter, bool isNegated)
  : iter_(iter), isNegated(isNegated) {
    this->currentValue = iter_->currentValue;
  }

  Row next() override {
    return this->currentValue = iter_->next();
  }

  Row skip_to(Row row) override {
    return this->currentValue = iter_->skip_to(row);
  }

  std::shared_ptr<IteratorInterface<Row>> iter_;
  bool isNegated;
};

/**
 * A more sophisticated version of IntersectionIterator that can handle negated iterators.
 */

template<class Row>
struct GeneralIntersectionIterator : public IteratorInterface<Row> {
  GeneralIntersectionIterator(std::vector<std::shared_ptr<NegatableIterator<Row>>>& iters) : iters_(iters) {
    if (iters_.size() == 0) {
      throw std::runtime_error("GeneralIntersectionIterator requires at least one iterator");
    }
    numNonNegatedIterators_ = 0;
    for (auto it : iters_) {
      numNonNegatedIterators_ += !it->isNegated;
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
      if (!iter->isNegated) {
        if (r < iter->currentValue) {
          r = iter->currentValue;
          numWithMax = 1;
        } else if (r == iter->currentValue) {
          ++numWithMax;
        }
      }
    }
    return std::make_pair(r, numWithMax);
  }
  Row skip_to(Row row) override {
    for (auto iter : iters_) {
      iter->skip_to(row);
    }
    std::pair<Row, size_t> x = this->_max_non_negated();

    while (true) {
      if (x.first == Row::largest()) {
        return this->currentValue = x.first;
      }
      if (x.second == numNonNegatedIterators_) {
        bool isMatch = true;
        for (auto iter : iters_) {
          if (iter->isNegated && iter->skip_to(x.first) == x.first) {
            isMatch = false;
            break;
          }
        }
        if (isMatch) {
          return this->currentValue = x.first;
        } else {
          for (auto iter : iters_) {
            iter->next();
          }
        }
      } else {
        for (auto iter : iters_) {
          iter->skip_to(x.first);
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
  std::vector<std::shared_ptr<NegatableIterator<Row>>>& iters_;
  size_t numNonNegatedIterators_;
};

}  // namespace cpot

#endif  // GENERAL_INTERSECTION_ITERATOR_H