#include <Python.h>

#include <cstdint>
#include <deque>
#include <fstream>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

#include "../common/InvertedIndex.h"
#include "../common/utils.h"
#include "MathyRow.h"

#define II InvertedIndex<MathyRow>

void destroy_index_object(PyObject *obj) {
  delete (II *)PyCapsule_GetPointer(obj, "InvertedIndex");
}

static PyObject *newIndex(PyObject *self, PyObject *args) {
  // parse arguments
  char *name;
  if (!PyArg_ParseTuple(args, "s", &name)) {
    return NULL;
  }
  std::string nameStr(name);
  return PyCapsule_New((void *) new II(nameStr), "InvertedIndex", destroy_index_object);
}

static II *unpack_index(PyObject *args) {
  PyObject* index = NULL;
  if(!PyArg_UnpackTuple(args, "funk", 1, 1, &index)) {
    return NULL;
  }
  return (II*)PyCapsule_GetPointer(index, "InvertedIndex");
}

static PyObject *currentMemoryUsed(PyObject *self, PyObject *args) {
  II *index = unpack_index(args);
  return Py_BuildValue("i", index->currentMemoryUsed());
}

static PyObject *insert(PyObject *self, PyObject *args) {
  PyObject* index = NULL;
  uint64_t token;
  long docid, value;
  if(!PyArg_ParseTuple(args, "OKll", &index, &token, &docid, &value)) {
    return Py_BuildValue("i", 0);
  }

  ((II*)PyCapsule_GetPointer(index, "InvertedIndex"))->insert(token, MathyRow{
    (uint32_t)docid,
    (uint32_t)value,
  });

  return Py_BuildValue("i", 1);
}

static PyObject *count(PyObject *self, PyObject *args) {
  PyObject* index = NULL;
  long token;
  if(!PyArg_ParseTuple(args, "Ol", &index, &token)) {
    return NULL;
  }

  uint64_t count = ((II*)PyCapsule_GetPointer(index, "InvertedIndex"))->count(token);

  return Py_BuildValue("i", count);
}

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

static PyObject *vector2obj(const std::vector<MathyRow>& rows) {
  PyObject *list = PyList_New(rows.size());
  if (list == NULL) {
    return NULL;
  }

  for (size_t i = 0; i < rows.size(); ++i) {
    PyList_SetItem(list, i,
      Py_BuildValue("(ll)", rows[i].docid, rows[i].value)
    );
  }
  return list;
}

static PyObject *fetch(PyObject *self, PyObject *args) {
  PyObject* index = NULL;
  uint64_t token;
  long docid, value, limit;
  if(!PyArg_ParseTuple(args, "OKlll", &index, &token, &docid, &value, &limit)) {
    return NULL;
  }

  std::shared_ptr<IteratorInterface<MathyRow>> it = ((II*)PyCapsule_GetPointer(index, "InvertedIndex"))->iterator(
    token,
    MathyRow{
      (uint32_t)docid,
      (uint32_t)value,
    }
  );

  return vector2obj(ffetch(it, limit));
}

static PyObject *flush(PyObject *self, PyObject *args) {
  II *index = unpack_index(args);
  index->flush();
  return Py_BuildValue("i", 1);
}

static PyObject *intersect(PyObject *self, PyObject *args) {
  PyObject* indexObj = NULL;
  PyObject *tokenList;
  long docid, value, limit;
  if(!PyArg_ParseTuple(args, "OOlll", &indexObj, &tokenList, &docid, &value, &limit)) {
    return NULL;
  }

  std::vector<uint64_t> tokens;
  const size_t n = PyList_GET_SIZE(tokenList);
  for (size_t i = 0; i < n; ++i) {
    PyObject *obj = PyList_GET_ITEM(tokenList, i);
    if (!PyLong_CheckExact(obj)) {
      return NULL;
    }
    long token = PyLong_AsUnsignedLongLong(obj);
    tokens.push_back(token);
  }

  II *index = (II*)PyCapsule_GetPointer(indexObj, "InvertedIndex");

  std::vector< std::shared_ptr<IteratorInterface<MathyRow>> > iters;
  for (long token : tokens) {
    iters.push_back(index->iterator(
      token,
      MathyRow{
        (uint32_t)docid,
        (uint32_t)value,
      }
    ));
  }

  IntersectionIterator<MathyRow> it(iters);

  return vector2obj(ffetch(&it, limit));
}

static PyMethodDef HelloMethods[] = {
 { "newIndex", newIndex, METH_VARARGS, "Say cpot" },
 { "currentMemoryUsed", currentMemoryUsed, METH_VARARGS, "Say cpot" },
 { "insert", insert, METH_VARARGS, "Say cpot" },
 { "flush", flush, METH_VARARGS, "Say cpot" },
 { "count", count, METH_VARARGS, "Say cpot" },
 { "fetch", fetch, METH_VARARGS, "Say cpot" },
 { "intersect", intersect, METH_VARARGS, "Say cpot" },
 { NULL, NULL, 0, NULL }
};

static struct PyModuleDef cpotmodule = {
    PyModuleDef_HEAD_INIT,
    "cpot",   /* name of module */
    NULL,
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    HelloMethods
};

PyMODINIT_FUNC
PyInit_cpot(void)
{
    PyObject *m;

    m = PyModule_Create(&cpotmodule);
    if (m == NULL) {
      return NULL;
    }

    return m;
}