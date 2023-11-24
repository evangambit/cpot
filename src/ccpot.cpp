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
#include "uint64/UInt64Row.h"
#include "mathy/MathyRow.h"

using namespace cpot;

enum class RowType {
  Undefined = 0,
  UInt64 = 1,
  Mathy = 2,
  Count = 3,
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
struct IndexNamer<MathyRow> {
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
  for (size_t i = 0; i < limit; ++i) {
    if (it->currentValue == T::largest()) {
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

template<class Row>
bool objectToRow(PyObject *object, Row *row) {
  return false;
}

template<>
bool objectToRow(PyObject *object, UInt64Row *row) {
  if (!PyLong_CheckExact(object)) {
    PyErr_SetString(PyExc_TypeError, "row is not an int");
    return false;
  }
  *row = PyLong_AsUnsignedLongLong(object);
  return true;
}

template<class Row>
PyObject *rowToObject(Row row) {
  return NULL;
}

template<>
PyObject *rowToObject(UInt64Row row) {
  return Py_BuildValue("K", row.val);
}

template<>
PyObject *rowToObject(MathyRow row) {
  return Py_BuildValue("(ll)", row.docid, row.value);
}

template<class Row>
PyObject *vector2obj(const std::vector<Row>& rows) {
  PyObject *list = PyList_New(rows.size());
  if (list == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < rows.size(); ++i) {
    PyList_SetItem(list, i, rowToObject(rows[i]));
  }
  return list;
}

template<>
bool objectToRow(PyObject *object, MathyRow *row) {
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
  *row = MathyRow{
    PyLong_AsUnsignedLongLong(docid),
    PyLong_AsUnsignedLongLong(value)
  };
  return row;
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
    return Py_BuildValue("k", index->currentMemoryUsed());
  }

  static PyObject *insert(PyObject *indexObj, uint64_t token, PyObject *rowObj) {
    InvertedIndex<Row> *index = (InvertedIndex<Row> *)PyCapsule_GetPointer(indexObj, IndexNamer<Row>::name());
    Row row;
    if (!objectToRow(rowObj, &row)) {
      PyErr_SetString(PyExc_TypeError, "invalid row");
      return NULL;
    }
    index->insert(token, row);
    Py_INCREF(Py_None);
    return Py_None;
  }

  static PyObject *count(PyObject *indexObj, uint64_t token) {
    InvertedIndex<Row> *index = (InvertedIndex<Row> *)PyCapsule_GetPointer(indexObj, IndexNamer<Row>::name());
    return Py_BuildValue("k", index->count(token));
  }

  static PyObject *flush(PyObject *indexObj) {
    InvertedIndex<Row> *index = (InvertedIndex<Row> *)PyCapsule_GetPointer(indexObj, IndexNamer<Row>::name());
    index->flush();
    Py_INCREF(Py_None);
    return Py_None;
  }

  static PyObject *intersect(PyObject *indexObj, PyObject *tokenList, PyObject *lowerBoundObj, uint64_t limit) {
    InvertedIndex<Row> *index = (InvertedIndex<Row> *)PyCapsule_GetPointer(indexObj, IndexNamer<Row>::name());

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

    return vector2obj(ffetch(&it, limit));
  }

  static PyObject *generalized_intersect(PyObject *indexObj, PyObject *tokenList, PyObject *lowerBoundObj, uint64_t limit) {

    InvertedIndex<Row> *index = (InvertedIndex<Row> *)PyCapsule_GetPointer(indexObj, IndexNamer<Row>::name());

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

    std::vector< std::shared_ptr<NegatableIterator<Row>> > iters;
    for (std::pair<uint64_t, bool> token : tokens) {
      iters.push_back(std::make_shared<NegatableIterator<Row>>(index->iterator(
        token.first,
        lowerBound
      ), token.second));
    }

    GeneralIntersectionIterator<Row> it(iters);

    return vector2obj(ffetch(&it, limit));
  }

  static PyObject *token_iterator(PyObject *indexObj, uint64_t token, PyObject *lowerBoundObj) {
    InvertedIndex<Row> *index = (InvertedIndex<Row> *)PyCapsule_GetPointer(indexObj, IndexNamer<Row>::name());
    Row lowerBound;
    if (!objectToRow(lowerBoundObj, &lowerBound)) {
      return NULL;
    }
    std::shared_ptr<IteratorInterface<Row>> iterator = index->iterator(token, lowerBound);

    return iterator_to_object(iterator);
  }

  static PyObject *fetch_many(PyObject *iteratorObj, uint64_t limit) {
    std::shared_ptr<IteratorInterface<Row>> iterator = object_to_iterator<Row>(iteratorObj);
    return vector2obj(ffetch(iterator, limit));
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
    case RowType::UInt64:
      return Index<UInt64Row>::newIndex(nameStr);
    case RowType::Mathy:
      return Index<MathyRow>::newIndex(nameStr);
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
    case RowType::UInt64:
      return Index<UInt64Row>::currentMemoryUsed(indexObj);
    case RowType::Mathy:
      return Index<MathyRow>::currentMemoryUsed(indexObj);
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
    case RowType::UInt64:
      return Index<UInt64Row>::insert(indexObj, token, rowObj);
    case RowType::Mathy:
      return Index<MathyRow>::insert(indexObj, token, rowObj);
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
    case RowType::UInt64:
      return Index<UInt64Row>::count(indexObj, token);
    case RowType::Mathy:
      return Index<MathyRow>::count(indexObj, token);
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
    case RowType::UInt64:
      return Index<UInt64Row>::flush(indexObj);
    case RowType::Mathy:
      return Index<MathyRow>::flush(indexObj);
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
    case RowType::UInt64:
      return Index<UInt64Row>::intersect(indexObj, tokenList, lowerBound, limit);
    case RowType::Mathy:
      return Index<MathyRow>::intersect(indexObj, tokenList, lowerBound, limit);
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
    case RowType::UInt64:
      return Index<UInt64Row>::generalized_intersect(indexObj, tokenList, lowerBound, limit);
    case RowType::Mathy:
      return Index<MathyRow>::generalized_intersect(indexObj, tokenList, lowerBound, limit);
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
    case RowType::UInt64:
      return Index<UInt64Row>::token_iterator(indexObj, token, lowerBound);
    case RowType::Mathy:
      return Index<MathyRow>::token_iterator(indexObj, token, lowerBound);
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
    case RowType::UInt64:
      return Index<UInt64Row>::fetch_many(iteratorObj, limit);
    case RowType::Mathy:
      return Index<MathyRow>::fetch_many(iteratorObj, limit);
    default:
      PyErr_SetString(PyExc_TypeError, "Invalid row type");
      return NULL;
  }
}

static PyMethodDef CcpotMethods[] = {
 { "newIndex", newIndex, METH_VARARGS, "Create a new index" },
 { "currentMemoryUsed", currentMemoryUsed, METH_VARARGS, "The amount of memory currently used" },
 { "insert", insert, METH_VARARGS, "Insert a token/doc pair" },
 { "flush", flush, METH_VARARGS, "Save the current changes to disk" },
 { "count", count, METH_VARARGS, "Returns how many times a token occurs" },
 { "intersect", intersect, METH_VARARGS, "Returns all documents associated with all of the given tokens" },
 { "generalized_intersect", generalized_intersect, METH_VARARGS, "Like intersect but takes (token, isNegated) tuples rather than simply tokens" },
 { "token_iterator", token_iterator, METH_VARARGS, "TODO" },
 { "fetch_many", fetch_many, METH_VARARGS, "TODO" },
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

    return m;
}