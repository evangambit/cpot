#include <Python.h>

#include <cstdint>
#include <deque>
#include <fstream>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/InvertedIndex.h"
#include "common/GeneralIntersectionIterator.h"
#include "common/KVUnionIterator.h"
#include "UInt64Row.h"
#include "UInt32PairRow.h"
#include "UInt64KeyValueRow.h"

#include "numpy/arrayobject.h"

using namespace cpot;

enum class RowType {
  Undefined = 0,
  UInt64Index = 1,
  UInt32PairIndex = 2,
  UInt64KeyValueIndex = 3,
  Count = 4,
};

template<class T>
struct IndexNamer {
  static const char* name() {
    return "";
  }
  static const char* iterator_name() {
    return "";
  }
};

template<>
struct IndexNamer<UInt64Row> {
  static const char* name() {
    return "UInt64Index";
  }
  static const char* iterator_name() {
    return "UInt64Iterator";
  }
};

template<>
struct IndexNamer<UInt32PairRow> {
  static const char* name() {
    return "MathyIndex";
  }
  static const char* iterator_name() {
    return "MathyIterator";
  }
};

template<class T>
std::vector<T> ffetch(IteratorInterface<T> *it, size_t limit) {
  std::vector<T> r;
  if (limit != size_t(-1)) {
    r.reserve(limit);
  }
  for (size_t i = 0; i < limit; ++i) {
    if (it->currentValue == T::largest()) {
      break;
    }
    r.push_back(it->currentValue);
    it->next();
  }
  return r;
}

// Specialization for KVUnionIterator
template<>
std::vector<std::pair<uint64_t, std::vector<uint64_t>>> ffetch(IteratorInterface<std::pair<uint64_t, std::vector<uint64_t>>> *it, size_t limit) {
  std::vector<std::pair<uint64_t, std::vector<uint64_t>>> r;
  if (limit != size_t(-1)) {
    r.reserve(limit);
  }
  for (size_t i = 0; i < limit; ++i) {
    if (it->currentValue.first == uint64_t(-1)) {
      break;
    }
    r.push_back(it->currentValue);
    it->next();
  }
  return r;
}

template<class T>
std::vector<T> ffetch(std::shared_ptr< IteratorInterface<T> > it, size_t limit) {
  return ffetch(it.get(), limit);
}

// template<class Row>
// bool objectToRow(PyObject *object, Row *row) {
//   return false;
// }

// template<class Row>
// PyObject *rowToObject(Row row) {
//   return NULL;
// }

bool objectToRow(PyObject *object, UInt64Row *row) {
  if (!PyLong_CheckExact(object)) {
    PyErr_SetString(PyExc_TypeError, "row is not an int");
    return false;
  }
  *row = PyLong_AsUnsignedLongLong(object);
  return true;
}

PyObject *rowToObject(UInt64Row row) {
  return Py_BuildValue("K", row.val);
}

PyObject *rowToObject(UInt32PairRow row) {
  return Py_BuildValue("(ll)", row.docid, row.value);
}

bool objectToRow(PyObject *object, UInt32PairRow *row) {
  if (!PyTuple_CheckExact(object)) {
    PyErr_SetString(PyExc_TypeError, "row is not a tuple");
    return false;
  }
  if (PyTuple_Size(object) != 2) {
    PyErr_SetString(PyExc_TypeError, "row has incorrect length");
    return false;
  }
  PyObject *docid = PyTuple_GetItem(object, 0);
  PyObject *value = PyTuple_GetItem(object, 1);
  if (!PyLong_CheckExact(docid)) {
    PyErr_SetString(PyExc_TypeError, "docid is not an int");
    return false;
  }
  if (!PyLong_CheckExact(value)) {
    PyErr_SetString(PyExc_TypeError, "value is not an int");
    return false;
  }
  *row = UInt32PairRow{
    (uint32_t)PyLong_AsUnsignedLong(docid),
    (uint32_t)PyLong_AsUnsignedLong(value)
  };
  return row;
}

PyObject *rowToObject(UInt64KeyValueRow row) {
  return Py_BuildValue("(KK)", row.key, row.value);
}

bool objectToRow(PyObject *object, UInt64KeyValueRow *row) {
  if (!PyTuple_CheckExact(object)) {
    PyErr_SetString(PyExc_TypeError, "row is not a tuple");
    return false;
  }
  if (PyTuple_Size(object) != 2) {
    PyErr_SetString(PyExc_TypeError, "row has incorrect length");
    return false;
  }
  PyObject *docid = PyTuple_GetItem(object, 0);
  PyObject *value = PyTuple_GetItem(object, 1);
  if (!PyLong_CheckExact(docid)) {
    PyErr_SetString(PyExc_TypeError, "docid is not an int");
    return false;
  }
  if (!PyLong_CheckExact(value)) {
    PyErr_SetString(PyExc_TypeError, "value is not an int");
    return false;
  }
  *row = UInt64KeyValueRow{
    PyLong_AsUnsignedLongLong(docid),
    PyLong_AsUnsignedLongLong(value)
  };
  return row;
}

// We could potentially use something like https://stackoverflow.com/a/76623718
// to "steal" the vector's data pointer, avoiding the O(n) std::memcopy
// operation. Given that memcpy is probably not the bottleneck, this is
// probably not worth the smell/complexity.

PyObject *vector2npy(const std::vector<UInt64Row>& rows) {
  npy_intp dims[1] = {(npy_intp)rows.size()};
  uint64_t *arr = new uint64_t[rows.size()];
  std::memcpy(arr, &rows[0], rows.size() * sizeof(UInt64Row));
  return PyArray_SimpleNewFromData(1, dims, NPY_UINT64, arr);
}

PyObject *vector2npy(const std::vector<UInt32PairRow>& rows) {
  npy_intp dims[2] = {(npy_intp)rows.size(), 2};
  uint32_t *arr = new uint32_t[rows.size() * 2];
  std::memcpy(arr, &rows[0], rows.size() * sizeof(UInt32PairRow));
  return PyArray_SimpleNewFromData(2, dims, NPY_UINT32, arr);
}

PyObject *vector2npy(const std::vector<UInt64KeyValueRow>& rows) {
  npy_intp dims[2] = {(npy_intp)rows.size(), 2};
  uint64_t *arr = new uint64_t[rows.size() * 2];
  std::memcpy(arr, &rows[0], rows.size() * sizeof(UInt64KeyValueRow));
  return PyArray_SimpleNewFromData(2, dims, NPY_UINT64, arr);
}


template<class Row>
static void destroy_index_object(PyObject *indexObj) {
  InvertedIndex<Row> *index = (InvertedIndex<Row> *)PyCapsule_GetPointer(indexObj, IndexNamer<Row>::name());
  delete index;
}

template<class T>
struct IteratorWrapper {
  std::shared_ptr<IteratorInterface<T>> ptr;
};

template<class Row>
void destroy_iterator_object(PyObject *object) {
  auto *wrapper = (IteratorWrapper<Row> *)PyCapsule_GetPointer(object, IndexNamer<Row>::iterator_name());
  delete wrapper;
}

template<class Row>
PyObject *iterator_to_object(std::shared_ptr<IteratorInterface<Row>> ptr) {
  auto *wrapper = new IteratorWrapper<Row>{ptr};
  return PyCapsule_New((void *)wrapper, IndexNamer<Row>::iterator_name(), destroy_iterator_object<Row>);
}

template<class Row>
std::shared_ptr<IteratorInterface<Row>> object_to_iterator(PyObject *object) {
  auto *wrapper = (IteratorWrapper<Row> *)PyCapsule_GetPointer(object, IndexNamer<Row>::iterator_name());
  if (wrapper == nullptr) {
    return std::shared_ptr<IteratorInterface<Row>>(nullptr);
  }
  return wrapper->ptr;
}

template<class Row>
struct Index {
  static PyObject *newIndex(std::string name) {
    InvertedIndex<Row> *index = new InvertedIndex<Row>(name);
    return PyCapsule_New((void *)index, IndexNamer<Row>::name(), destroy_index_object<Row>);
  }

  static PyObject *currentMemoryUsed(PyObject *indexObj) {
    InvertedIndex<Row> *index = (InvertedIndex<Row> *)PyCapsule_GetPointer(indexObj, IndexNamer<Row>::name());
    if (index == nullptr) {
      PyErr_SetString(PyExc_TypeError, "invalid index");
      return NULL;
    }
    return Py_BuildValue("k", index->currentMemoryUsed());
  }

  static PyObject *insert(PyObject *indexObj, uint64_t token, PyObject *rowObj) {
    InvertedIndex<Row> *index = (InvertedIndex<Row> *)PyCapsule_GetPointer(indexObj, IndexNamer<Row>::name());
    if (index == nullptr) {
      PyErr_SetString(PyExc_TypeError, "invalid index");
      return NULL;
    }
    Row row;
    if (!objectToRow(rowObj, &row)) {
      PyErr_SetString(PyExc_TypeError, "invalid row");
      return NULL;
    }
    index->insert(token, row);
    Py_INCREF(Py_None);
    return Py_None;
  }

  static PyObject *remove(PyObject *indexObj, uint64_t token, PyObject *rowObj) {
    InvertedIndex<Row> *index = (InvertedIndex<Row> *)PyCapsule_GetPointer(indexObj, IndexNamer<Row>::name());
    if (index == nullptr) {
      PyErr_SetString(PyExc_TypeError, "invalid index");
      return NULL;
    }
    Row row;
    if (!objectToRow(rowObj, &row)) {
      PyErr_SetString(PyExc_TypeError, "invalid row");
      return NULL;
    }
    bool result = index->remove(token, row);
    return PyBool_FromLong(result);
  }

  static PyObject *count(PyObject *indexObj, uint64_t token) {
    InvertedIndex<Row> *index = (InvertedIndex<Row> *)PyCapsule_GetPointer(indexObj, IndexNamer<Row>::name());
    if (index == nullptr) {
      PyErr_SetString(PyExc_TypeError, "invalid index");
      return NULL;
    }
    return Py_BuildValue("k", index->count(token));
  }

  static PyObject *flush(PyObject *indexObj) {
    InvertedIndex<Row> *index = (InvertedIndex<Row> *)PyCapsule_GetPointer(indexObj, IndexNamer<Row>::name());
    if (index == nullptr) {
      PyErr_SetString(PyExc_TypeError, "invalid index");
      return NULL;
    }
    index->flush();
    Py_INCREF(Py_None);
    return Py_None;
  }

  static PyObject *intersect(PyObject *indexObj, PyObject *tokenList, PyObject *lowerBoundObj, uint64_t limit) {
    InvertedIndex<Row> *index = (InvertedIndex<Row> *)PyCapsule_GetPointer(indexObj, IndexNamer<Row>::name());
    if (index == nullptr) {
      PyErr_SetString(PyExc_TypeError, "invalid index");
      return NULL;
    }

    Row lowerBound;
    if (!objectToRow(lowerBoundObj, &lowerBound)) {
      PyErr_SetString(PyExc_TypeError, "invalid lower bound");
      return NULL;
    }

    std::vector<uint64_t> tokens;
    const size_t n = PyList_GET_SIZE(tokenList);
    for (size_t i = 0; i < n; ++i) {
      PyObject *obj = PyList_GET_ITEM(tokenList, i);
      if (!PyLong_CheckExact(obj)) {
        PyErr_SetString(PyExc_TypeError, "invalid token");
        return NULL;
      }
      uint64_t token = PyLong_AsUnsignedLongLong(obj);
      tokens.push_back(token);
    }

    std::vector< std::shared_ptr<IteratorInterface<Row>> > iters;
    for (uint64_t token : tokens) {
      iters.push_back(index->iterator(
        token,
        lowerBound
      ));
    }

    IntersectionIterator<Row> it(iters);

    return vector2npy(ffetch(&it, limit));
  }

  static PyObject *generalized_intersect(PyObject *indexObj, PyObject *tokenList, PyObject *lowerBoundObj, uint64_t limit) {

    InvertedIndex<Row> *index = (InvertedIndex<Row> *)PyCapsule_GetPointer(indexObj, IndexNamer<Row>::name());
    if (index == nullptr) {
      PyErr_SetString(PyExc_TypeError, "invalid index");
      return NULL;
    }

    Row lowerBound;
    if (!objectToRow(lowerBoundObj, &lowerBound)) {
      return NULL;
    }

    std::vector<std::pair<uint64_t, bool>> tokens;
    const size_t n = PyList_GET_SIZE(tokenList);
    for (size_t i = 0; i < n; ++i) {
      PyObject *tuple = PyList_GET_ITEM(tokenList, i);
      if (!PyTuple_CheckExact(tuple)) {
        PyErr_SetString(PyExc_TypeError, "Token is not a tuple");
        return NULL;
      }
      if (PyTuple_Size(tuple) != 2) {
        PyErr_SetString(PyExc_TypeError, "Token is not the correct length");
        return NULL;
      }
      PyObject *tokenObj = PyTuple_GetItem(tuple, 0);
      PyObject *negatedObj = PyTuple_GetItem(tuple, 1);

      if (!PyLong_CheckExact(tokenObj)) {
        PyErr_SetString(PyExc_TypeError, "Token[0] is not an int");
        return NULL;
      }
      uint64_t token = PyLong_AsUnsignedLongLong(tokenObj);

      if (!PyBool_Check(negatedObj)) {
        PyErr_SetString(PyExc_TypeError, "Token[1] is not a boolean");
        return NULL;
      }
      bool isNegated = PyObject_IsTrue(negatedObj);

      tokens.push_back(std::make_pair(token, isNegated));
    }

    std::vector<std::pair<std::shared_ptr<IteratorInterface<Row>>, bool>> iters;
    size_t numNonNegated = 0;
    for (std::pair<uint64_t, bool> token : tokens) {
      iters.push_back(std::make_pair(index->iterator(
        token.first,
        lowerBound
      ), token.second));
      if (!token.second) {
        ++numNonNegated;
      }
    }

    if (numNonNegated == 0) {
      PyErr_SetString(PyExc_TypeError, "At least one token must not be negated.");
      return NULL;
    }

    GeneralIntersectionIterator<Row> it(iters);

    return vector2npy(ffetch(&it, limit));
  }

  static PyObject *token_iterator(PyObject *indexObj, uint64_t token, PyObject *lowerBoundObj) {
    InvertedIndex<Row> *index = (InvertedIndex<Row> *)PyCapsule_GetPointer(indexObj, IndexNamer<Row>::name());
    if (index == nullptr) {
      PyErr_SetString(PyExc_TypeError, "invalid index");
      return NULL;
    }
    Row lowerBound;
    if (!objectToRow(lowerBoundObj, &lowerBound)) {
      return NULL;
    }
    std::shared_ptr<IteratorInterface<Row>> iterator = index->iterator(token, lowerBound);

    return iterator_to_object(iterator);
  }

  static PyObject *generalized_intersection_iterator(PyObject *iteratorList) {
    std::vector<std::pair<std::shared_ptr<IteratorInterface<Row>>, bool>> iterators;

    const size_t n = PyList_GET_SIZE(iteratorList);
    for (size_t i = 0; i < n; ++i) {
      PyObject *tuple = PyList_GET_ITEM(iteratorList, i);
      if (!PyTuple_CheckExact(tuple)) {
        PyErr_SetString(PyExc_TypeError, "Iterator is not a tuple");
        return NULL;
      }
      if (PyTuple_Size(tuple) != 2) {
        PyErr_SetString(PyExc_TypeError, "Iterator is not the correct length");
        return NULL;
      }
      PyObject *iteratorObj = PyTuple_GetItem(tuple, 0);
      PyObject *negatedObj = PyTuple_GetItem(tuple, 1);

      if (!PyCapsule_CheckExact(iteratorObj)) {
        PyErr_SetString(PyExc_TypeError, "Iterator[0] is not a capsule");
        return NULL;
      }
      if (!PyBool_Check(negatedObj)) {
        PyErr_SetString(PyExc_TypeError, "Iterator[1] is not a boolean");
        return NULL;
      }

      bool isNegated = PyObject_IsTrue(negatedObj);

      iterators.push_back(std::make_pair(object_to_iterator<Row>(iteratorObj), isNegated));
      if (!iterators.back().first) {
        PyErr_SetString(PyExc_TypeError, "Object is not an iterator");
        return NULL;
      }
    }

    auto iterator = std::make_shared<GeneralIntersectionIterator<Row>>(iterators);
    return iterator_to_object<Row>(iterator);
  }

  static PyObject *union_iterator(PyObject *iteratorList) {
    std::vector<std::shared_ptr<IteratorInterface<Row>>> iterators;
    const size_t n = PyList_GET_SIZE(iteratorList);
    for (size_t i = 0; i < n; ++i) {
      PyObject *iteratorObj = PyList_GET_ITEM(iteratorList, i);
      if (!PyCapsule_CheckExact(iteratorObj)) {
        PyErr_SetString(PyExc_TypeError, "Iterator is not a capsule");
        return NULL;
      }
      iterators.push_back(object_to_iterator<Row>(iteratorObj));
      if (!iterators.back()) {
        PyErr_SetString(PyExc_TypeError, "Object is not an iterator");
        return NULL;
      }
    }
    auto iterator = std::make_shared<UnionIterator<Row>>(iterators);
    return iterator_to_object<Row>(iterator);
  }

  static PyObject *empty_iterator() {
    auto iterator = std::make_shared<VectorIterator<Row>>(std::vector<Row>());
    return iterator_to_object<Row>(iterator);
  }

  static PyObject *fetch_many(PyObject *iteratorObj, uint64_t limit) {
    std::shared_ptr<IteratorInterface<Row>> iterator = object_to_iterator<Row>(iteratorObj);
    if (!iterator) {
      PyErr_SetString(PyExc_TypeError, "Object is not an iterator");
      return NULL;
    }
    std::vector<Row> rows = ffetch(iterator, limit);
    return vector2npy(rows);
  }

  // Assumes key and value are uint64_t
  static PyObject *kv_union64(PyObject *indexObj, PyObject *tokenList) {
    InvertedIndex<Row> *index = (InvertedIndex<Row> *)PyCapsule_GetPointer(indexObj, IndexNamer<Row>::name());
    if (index == nullptr) {
      PyErr_SetString(PyExc_TypeError, "invalid index");
      return NULL;
    }

    std::vector<uint64_t> tokens;
    const size_t n = PyList_GET_SIZE(tokenList);
    for (size_t i = 0; i < n; ++i) {
      PyObject *obj = PyList_GET_ITEM(tokenList, i);
      if (!PyLong_CheckExact(obj)) {
        PyErr_SetString(PyExc_TypeError, "invalid token");
        return NULL;
      }
      uint64_t token = PyLong_AsUnsignedLongLong(obj);
      tokens.push_back(token);
    }

    std::vector< std::shared_ptr<IteratorInterface<Row>> > iters;
    for (uint64_t token : tokens) {
      iters.push_back(index->iterator(
        token,
        Row::smallest()
      ));
    }

    auto it = KVUnionIterator<Row, uint64_t, uint64_t>(iters);
    std::vector<std::pair<uint64_t, std::vector<uint64_t>>> rows = ffetch(&it, size_t(-1));

    // [key, value1, value2, ...]
    const int kNumColumns = 1 + iters.size();

    npy_intp dims[2] = {(npy_intp)rows.size(), kNumColumns};
    uint64_t *arr = new uint64_t[rows.size() * kNumColumns];
    for (size_t i = 0; i < rows.size(); ++i) {
      arr[i * kNumColumns + 0] = rows[i].first;
      assert(rows[i].second.size() == iters.size());
      for (size_t j = 0; j < iters.size(); ++j) {
        arr[i * kNumColumns + j + 1] = rows[i].second[j];
      }
    }
    return PyArray_SimpleNewFromData(2, dims, NPY_UINT64, arr);
  }

};

static PyObject *newIndex(PyObject *self, PyObject *args) {
  uint64_t rowTypeInt;
  char *name;
  if (!PyArg_ParseTuple(args, "Ks", &rowTypeInt, &name)) {
    return NULL;
  }
  std::string nameStr(name);

  switch (RowType(rowTypeInt)) {
    case RowType::UInt64Index:
      return Index<UInt64Row>::newIndex(nameStr);
    case RowType::UInt32PairIndex:
      return Index<UInt32PairRow>::newIndex(nameStr);
    case RowType::UInt64KeyValueIndex:
      return Index<UInt64KeyValueRow>::newIndex(nameStr);
    default:
      PyErr_SetString(PyExc_TypeError, "Invalid row type");
      return NULL;
  }
}

static PyObject *currentMemoryUsed(PyObject *self, PyObject *args) {
  uint64_t rowTypeInt;
  PyObject* indexObj = NULL;

  if(!PyArg_ParseTuple(args, "KO", &rowTypeInt, &indexObj)) {
    PyErr_SetString(PyExc_TypeError, "Invalid args");
    return NULL;
  }

  switch (RowType(rowTypeInt)) {
    case RowType::UInt64Index:
      return Index<UInt64Row>::currentMemoryUsed(indexObj);
    case RowType::UInt32PairIndex:
      return Index<UInt32PairRow>::currentMemoryUsed(indexObj);
    case RowType::UInt64KeyValueIndex:
      return Index<UInt64KeyValueRow>::currentMemoryUsed(indexObj);
    default:
      PyErr_SetString(PyExc_TypeError, "Invalid row type");
      return NULL;
  }
}

static PyObject *insert(PyObject *self, PyObject *args) {
  PyObject *indexObj = NULL;
  PyObject *rowObj;
  uint64_t token;
  uint64_t rowTypeInt;
  if(!PyArg_ParseTuple(args, "KOKO", &rowTypeInt, &indexObj, &token, &rowObj)) {
    PyErr_SetString(PyExc_TypeError, "Invalid args");
    return NULL;
  }

  switch (RowType(rowTypeInt)) {
    case RowType::UInt64Index:
      return Index<UInt64Row>::insert(indexObj, token, rowObj);
    case RowType::UInt32PairIndex:
      return Index<UInt32PairRow>::insert(indexObj, token, rowObj);
    case RowType::UInt64KeyValueIndex:
      return Index<UInt64KeyValueRow>::insert(indexObj, token, rowObj);
    default:
      PyErr_SetString(PyExc_TypeError, "Invalid row type");
      return NULL;
  }
}

static PyObject *remove(PyObject *self, PyObject *args) {
  PyObject *indexObj = NULL;
  PyObject *rowObj;
  uint64_t token;
  uint64_t rowTypeInt;
  if(!PyArg_ParseTuple(args, "KOKO", &rowTypeInt, &indexObj, &token, &rowObj)) {
    PyErr_SetString(PyExc_TypeError, "Invalid args");
    return NULL;
  }

  switch (RowType(rowTypeInt)) {
    case RowType::UInt64Index:
      return Index<UInt64Row>::remove(indexObj, token, rowObj);
    case RowType::UInt32PairIndex:
      return Index<UInt32PairRow>::remove(indexObj, token, rowObj);
    case RowType::UInt64KeyValueIndex:
      return Index<UInt64KeyValueRow>::remove(indexObj, token, rowObj);
    default:
      PyErr_SetString(PyExc_TypeError, "Invalid row type");
      return NULL;
  }
}

static PyObject *count(PyObject *self, PyObject *args) {
  PyObject *indexObj = NULL;
  uint64_t token;
  uint64_t rowTypeInt;
  if(!PyArg_ParseTuple(args, "KOK", &rowTypeInt, &indexObj, &token)) {
    PyErr_SetString(PyExc_TypeError, "Invalid args");
    return NULL;
  }

  switch (RowType(rowTypeInt)) {
    case RowType::UInt64Index:
      return Index<UInt64Row>::count(indexObj, token);
    case RowType::UInt32PairIndex:
      return Index<UInt32PairRow>::count(indexObj, token);
    case RowType::UInt64KeyValueIndex:
      return Index<UInt64KeyValueRow>::count(indexObj, token);
    default:
      PyErr_SetString(PyExc_TypeError, "Invalid row type");
      return NULL;
  }
}

static PyObject *flush(PyObject *self, PyObject *args) {
  PyObject *indexObj = NULL;
  uint64_t rowTypeInt;
  if(!PyArg_ParseTuple(args, "KO", &rowTypeInt, &indexObj)) {
    PyErr_SetString(PyExc_TypeError, "Invalid args");
    return NULL;
  }

  switch (RowType(rowTypeInt)) {
    case RowType::UInt64Index:
      return Index<UInt64Row>::flush(indexObj);
    case RowType::UInt32PairIndex:
      return Index<UInt32PairRow>::flush(indexObj);
    case RowType::UInt64KeyValueIndex:
      return Index<UInt64KeyValueRow>::flush(indexObj);
    default:
      PyErr_SetString(PyExc_TypeError, "Invalid row type");
      return NULL;
  }
}

static PyObject *intersect(PyObject *self, PyObject *args) {
  PyObject* indexObj = NULL;
  PyObject *tokenList;
  PyObject *lowerBound;
  uint64_t rowTypeInt;
  uint64_t limit;
  if(!PyArg_ParseTuple(args, "KOOOK", &rowTypeInt, &indexObj, &tokenList, &lowerBound, &limit)) {
    PyErr_SetString(PyExc_TypeError, "Invalid args");
    return NULL;
  }

  switch (RowType(rowTypeInt)) {
    case RowType::UInt64Index:
      return Index<UInt64Row>::intersect(indexObj, tokenList, lowerBound, limit);
    case RowType::UInt32PairIndex:
      return Index<UInt32PairRow>::intersect(indexObj, tokenList, lowerBound, limit);
    case RowType::UInt64KeyValueIndex:
      return Index<UInt64KeyValueRow>::intersect(indexObj, tokenList, lowerBound, limit);
    default:
      PyErr_SetString(PyExc_TypeError, "Invalid row type");
      return NULL;
  }
}

static PyObject *generalized_intersect(PyObject *self, PyObject *args) {
  PyObject* indexObj = NULL;
  PyObject *tokenList;
  PyObject *lowerBound;
  uint64_t rowTypeInt;
  uint64_t limit;

  if(!PyArg_ParseTuple(args, "KOOOK", &rowTypeInt, &indexObj, &tokenList, &lowerBound, &limit)) {
    PyErr_SetString(PyExc_TypeError, "Invalid args");
    return NULL;
  }

  switch (RowType(rowTypeInt)) {
    case RowType::UInt64Index:
      return Index<UInt64Row>::generalized_intersect(indexObj, tokenList, lowerBound, limit);
    case RowType::UInt32PairIndex:
      return Index<UInt32PairRow>::generalized_intersect(indexObj, tokenList, lowerBound, limit);
    case RowType::UInt64KeyValueIndex:
      return Index<UInt64KeyValueRow>::generalized_intersect(indexObj, tokenList, lowerBound, limit);
    default:
      PyErr_SetString(PyExc_TypeError, "Invalid row type");
      return NULL;
  }
}

static PyObject *token_iterator(PyObject *self, PyObject *args) {
  uint64_t rowTypeInt;
  PyObject* indexObj = NULL;
  uint64_t token;
  PyObject *lowerBound;
  if(!PyArg_ParseTuple(args, "KOKO", &rowTypeInt, &indexObj, &token, &lowerBound)) {
    PyErr_SetString(PyExc_TypeError, "Invalid args");
    return NULL;
  }

  switch (RowType(rowTypeInt)) {
    case RowType::UInt64Index:
      return Index<UInt64Row>::token_iterator(indexObj, token, lowerBound);
    case RowType::UInt32PairIndex:
      return Index<UInt32PairRow>::token_iterator(indexObj, token, lowerBound);
    case RowType::UInt64KeyValueIndex:
      return Index<UInt64KeyValueRow>::token_iterator(indexObj, token, lowerBound);
    default:
      PyErr_SetString(PyExc_TypeError, "Invalid row type");
      return NULL;
  }
}

static PyObject *generalized_intersection_iterator(PyObject *self, PyObject *args) {
  uint64_t rowTypeInt;
  PyObject* tokenList;
  if(!PyArg_ParseTuple(args, "KO", &rowTypeInt, &tokenList)) {
    PyErr_SetString(PyExc_TypeError, "Invalid args");
    return NULL;
  }

  switch (RowType(rowTypeInt)) {
    case RowType::UInt64Index:
      return Index<UInt64Row>::generalized_intersection_iterator(tokenList);
    case RowType::UInt32PairIndex:
      return Index<UInt32PairRow>::generalized_intersection_iterator(tokenList);
    case RowType::UInt64KeyValueIndex:
      return Index<UInt64KeyValueRow>::generalized_intersection_iterator(tokenList);
    default:
      PyErr_SetString(PyExc_TypeError, "Invalid row type");
      return NULL;
  }
}

static PyObject *union_iterator(PyObject *self, PyObject *args) {
  uint64_t rowTypeInt;
  PyObject* tokenList;
  if(!PyArg_ParseTuple(args, "KO", &rowTypeInt, &tokenList)) {
    PyErr_SetString(PyExc_TypeError, "Invalid args");
    return NULL;
  }
  switch (RowType(rowTypeInt)) {
    case RowType::UInt64Index:
      return Index<UInt64Row>::union_iterator(tokenList);
    case RowType::UInt32PairIndex:
      return Index<UInt32PairRow>::union_iterator(tokenList);
    case RowType::UInt64KeyValueIndex:
      return Index<UInt64KeyValueRow>::union_iterator(tokenList);
    default:
      PyErr_SetString(PyExc_TypeError, "Invalid row type");
      return NULL;
  }
}

static PyObject *empty_iterator(PyObject *self, PyObject *args) {
  uint64_t rowTypeInt;
  if(!PyArg_ParseTuple(args, "K", &rowTypeInt)) {
    PyErr_SetString(PyExc_TypeError, "Invalid args");
    return NULL;
  }
  switch (RowType(rowTypeInt)) {
    case RowType::UInt64Index:
      return Index<UInt64Row>::empty_iterator();
    case RowType::UInt32PairIndex:
      return Index<UInt32PairRow>::empty_iterator();
    case RowType::UInt64KeyValueIndex:
      return Index<UInt64KeyValueRow>::empty_iterator();
    default:
      PyErr_SetString(PyExc_TypeError, "Invalid row type");
      return NULL;
  }
}

static PyObject *fetch_many(PyObject *self, PyObject *args) {
  uint64_t rowTypeInt;
  PyObject* iteratorObj = NULL;
  uint64_t limit;
  if(!PyArg_ParseTuple(args, "KOK", &rowTypeInt, &iteratorObj, &limit)) {
    PyErr_SetString(PyExc_TypeError, "Invalid args");
    return NULL;
  }

  switch (RowType(rowTypeInt)) {
    case RowType::UInt64Index:
      return Index<UInt64Row>::fetch_many(iteratorObj, limit);
    case RowType::UInt32PairIndex:
      return Index<UInt32PairRow>::fetch_many(iteratorObj, limit);
    case RowType::UInt64KeyValueIndex:
      return Index<UInt64KeyValueRow>::fetch_many(iteratorObj, limit);
    default:
      PyErr_SetString(PyExc_TypeError, "Invalid row type");
      return NULL;
  }
}

static PyObject *kv_union(PyObject *self, PyObject *args) {
  uint64_t rowTypeInt;
  PyObject* indexObj = NULL;
  PyObject *tokenList;
  if(!PyArg_ParseTuple(args, "KOO", &rowTypeInt, &indexObj, &tokenList)) {
    PyErr_SetString(PyExc_TypeError, "Invalid args");
    return NULL;
  }

  switch (RowType(rowTypeInt)) {
    case RowType::UInt64KeyValueIndex:
      return Index<UInt64KeyValueRow>::kv_union64(indexObj, tokenList);
    case RowType::UInt64Index:
    case RowType::UInt32PairIndex:
      PyErr_SetString(PyExc_TypeError, "Row type does not support kv_union (not a key-value row type)");
      return NULL;
    default:
      PyErr_SetString(PyExc_TypeError, "Invalid row type");
      return NULL;
  }
}

static PyMethodDef CcpotMethods[] = {
 { "newIndex", newIndex, METH_VARARGS, "Create a new index." },
 { "currentMemoryUsed", currentMemoryUsed, METH_VARARGS, "The amount of memory currently used." },
 { "insert", insert, METH_VARARGS, "Insert a token/doc pair." },
 { "remove", remove, METH_VARARGS, "Delete a token/doc pair." },
 { "flush", flush, METH_VARARGS, "Save the current changes to disk." },
 { "count", count, METH_VARARGS, "Returns how many times a token occurs." },
 { "intersect", intersect, METH_VARARGS, "Returns all objects associated with all of the given tokens." },
 { "generalized_intersect", generalized_intersect, METH_VARARGS, "Like intersect but takes (token, isNegated) tuples rather than simply tokens" },
 { "token_iterator", token_iterator, METH_VARARGS, "Returns an iterator that loops over all objects associated with a given token." },
 { "generalized_intersection_iterator", generalized_intersection_iterator, METH_VARARGS, "Given a list of (iter: Iterator, isNegated: bool) tuples, returns an iterator that is the intersection of them all." },
 { "union_iterator", union_iterator, METH_VARARGS, "Given a list of iterators, returns an iterator that is the union of them all." },
 { "fetch_many", fetch_many, METH_VARARGS, "Pops N objects off of an iterator" },
 { "empty_iterator", empty_iterator, METH_VARARGS, "Returns an iterator that contains nothing" },
 { "kv_union", kv_union, METH_VARARGS, "TODO" },
 { NULL, NULL, 0, NULL }
};

static struct PyModuleDef ccpotmodule = {
    PyModuleDef_HEAD_INIT,
    "ccpot",   /* name of module */
    NULL,
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    CcpotMethods
};

PyMODINIT_FUNC
PyInit_ccpot(void)
{
    PyObject *m;

    m = PyModule_Create(&ccpotmodule);
    if (m == NULL) {
      return NULL;
    }

    import_array();

    return m;
}