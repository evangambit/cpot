#ifndef ITERATOR_H
#define ITERATOR_H

#include <vector>

namespace cpot {

template<class T>
struct IteratorInterface {
  T currentValue;

  // Returns the smallest value that is greater than or equal to val
  virtual T skip_to(T val) = 0;

  virtual T next() = 0;
};

template<class T>
T max(const std::vector<T>& A) {
  assert(A.size() > 0);
  T r = A[0];
  for (size_t i = 1; i < A.size(); ++i) {
    if (r < A[i]) {
      r = A[i];
    }
  }
  return r;
}

template<class T>
struct VectorIterator : public IteratorInterface<T> {
  VectorIterator(std::vector<T> data) : data(data) {
    if (data.size() == 0) {
      this->currentValue = T::largest();
    } else {
      this->currentValue = data[0];
    }
  }
  T skip_to(T row) override {
    auto it = std::lower_bound(this->data.begin(), this->data.end(), row);
    if (it == data.end()) {
      return this->currentValue = T::largest();
    }
    return this->currentValue = *it;
  }

  // Trivial implementation for testing purposes.
  T next() override {
    auto it = std::lower_bound(this->data.begin(), this->data.end(), this->currentValue);
    if (it == data.end() || it + 1 == data.end()) {
      return this->currentValue = T::largest();
    }
    return this->currentValue = *(it + 1);
  }
 private:
  std::vector<T> data;
};

template<class Row>
struct IntersectionIterator : public IteratorInterface<Row> {
  IntersectionIterator(std::vector<std::shared_ptr<IteratorInterface<Row>>>& iters) : iters(iters) {
    if (iters.size() == 0) {
      throw std::runtime_error("IntersectionIterator requires at least one iterator");
    }
    this->skip_to(Row::smallest());
  }
  Row skip_to(Row row) override {
    std::vector<Row> vals(iters.size());
    for (size_t i = 0; i < iters.size(); ++i) {
      vals[i] = iters[i]->skip_to(row);
    }
    Row x = max(vals);

    while (true) {
      for (size_t i = 0; i < iters.size(); ++i) {
        if (vals[i] < x) {
          vals[i] = iters[i]->skip_to(x);
        }
      }
      Row y = max(vals);
      if (x == y) {
        this->currentValue = x;
        return this->currentValue;
      }
      if (y == Row::largest()) {
        this->currentValue = y;
        return this->currentValue;
      }
      x = y;
    }
  }
  Row next() override {
    // TODO: make this more efficient.
    this->skip_to(this->currentValue.next());
    return this->currentValue;
  }
  const std::vector<std::shared_ptr<IteratorInterface<Row>>> iters;
};

template<class Row>
struct UnionIterator : public IteratorInterface<Row> {
  UnionIterator(std::vector<std::shared_ptr<IteratorInterface<Row>>>& iters) : iters(iters) {
    if (iters.size() == 0) {
      throw std::runtime_error("UnionIterator requires at least one iterator");
    }
    this->skip_to(Row::smallest());
  }
  Row skip_to(Row row) override {
    Row result = Row::smallest();

    Row lowest = Row::largest();
    for (size_t i = 0; i < iters.size(); ++i) {
      Row v = iters[i]->skip_to(row);
      if (v < lowest) {
        lowest = v;
      }
    }
    this->currentValue = lowest;
    return this->currentValue;
  }
  Row next() override {
    Row low = this->__lowest();
    if (low == Row::largest()) {
      this->currentValue = Row::largest();
      return this->currentValue;
    }
    for (size_t i = 0; i < this->iters.size(); ++i) {
      if (this->iters[i]->currentValue == low) {
        this->iters[i]->next();
      }
    }
    this->currentValue = this->__lowest();
    return this->currentValue;
  }
 private:
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

template<class Row>
void print_iterator(std::shared_ptr<IteratorInterface<Row>> it) {
  if (it->currentValue == Row::smallest()) {
    it->next();
  }
  while (it->currentValue < Row::largest()) {
    std::cout << " " << it->currentValue;
    it->next();
  }
  std::cout << std::endl;
}

}  // namespace cpot

#endif  // ITERATOR_H