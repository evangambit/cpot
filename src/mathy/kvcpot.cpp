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
// #include "../common/utils.h"
#include "MathyRow.h"

using namespace cpot;

typedef InvertedIndex<MathyRow> KVIndex;
const char *kKVIndexName = "KeyValueIndex";

static InvertedIndex<MathyRow> *unpack_index(PyObject *indexObj) {
  return (KVIndex *)PyCapsule_GetPointer(indexObj, kKVIndexName);
}

static void destroy_index_object(PyObject *obj) {
  delete unpack_index(obj);
}

static PyObject *pack_index(KVIndex *index) {
  return PyCapsule_New((void *)index, kKVIndexName, destroy_index_object);
}

static InvertedIndex<MathyRow> *args_to_index(PyObject *args) {
  PyObject *indexObj = NULL;
  if (!PyArg_ParseTuple(args, "O", &indexObj)) {
    throw std::runtime_error("Invalid arguments");
  }
  return unpack_index(indexObj);
}

static PyObject *newIndex(PyObject *self, PyObject *args) {
  // parse arguments
  char *name;
  if (!PyArg_ParseTuple(args, "s", &name)) {
    return NULL;
  }
  std::string nameStr(name);
  return pack_index(new KVIndex(nameStr));
}

static PyObject *currentMemoryUsed(PyObject *self, PyObject *args) {
  PyObject* index = NULL;
  if(!PyArg_UnpackTuple(args, "funk", 1, 1, &index)) {
    return NULL;
  }
  return Py_BuildValue("K", unpack_index(index)->currentMemoryUsed());
}

static PyObject *insert(PyObject *self, PyObject *args) {
  PyObject* index = NULL;
  uint64_t token;
  uint64_t docid, value;
  if(!PyArg_ParseTuple(args, "OKKK", &index, &token, &docid, &value)) {
    return Py_BuildValue("i", 0);
  }

  ((KVIndex*)PyCapsule_GetPointer(index, kKVIndexName))->insert(token, MathyRow{
    (uint32_t)docid,
    (uint32_t)value,
  });

  return Py_BuildValue("i", 1);
}

static PyObject *count(PyObject *self, PyObject *args) {
  PyObject* index = NULL;
  uint64_t token;
  if(!PyArg_ParseTuple(args, "OK", &index, &token)) {
    return NULL;
  }

  uint64_t count = ((KVIndex*)PyCapsule_GetPointer(index, kKVIndexName))->count(token);

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
  uint64_t docid, value, limit;
  if(!PyArg_ParseTuple(args, "OKKKK", &index, &token, &docid, &value, &limit)) {
    return NULL;
  }

  std::shared_ptr<IteratorInterface<MathyRow>> it = ((KVIndex*)PyCapsule_GetPointer(index, kKVIndexName))->iterator(
    token,
    MathyRow{
      (uint32_t)docid,
      (uint32_t)value,
    }
  );

  return vector2obj(ffetch(it, limit));
}

static PyObject *flush(PyObject *self, PyObject *args) {
  KVIndex *index = args_to_index(args);
  index->flush();
  return Py_BuildValue("i", 1);
}

static PyObject *intersect(PyObject *self, PyObject *args) {
  PyObject* indexObj = NULL;
  PyObject *tokenList;
  uint64_t docid, value, limit;
  if(!PyArg_ParseTuple(args, "OOKKK", &indexObj, &tokenList, &docid, &value, &limit)) {
    return NULL;
  }

  std::vector<uint64_t> tokens;
  const size_t n = PyList_GET_SIZE(tokenList);
  for (size_t i = 0; i < n; ++i) {
    PyObject *obj = PyList_GET_ITEM(tokenList, i);
    if (!PyLong_CheckExact(obj)) {
      return NULL;
    }
    uint64_t token = PyLong_AsUnsignedLongLong(obj);
    tokens.push_back(token);
  }

  KVIndex *index = (KVIndex*)PyCapsule_GetPointer(indexObj, kKVIndexName);

  std::vector< std::shared_ptr<IteratorInterface<MathyRow>> > iters;
  for (uint64_t token : tokens) {
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

static PyMethodDef KVCpotMethods[] = {
 { "newIndex", newIndex, METH_VARARGS, "Create a new index" },
 { "currentMemoryUsed", currentMemoryUsed, METH_VARARGS, "The amount of memory currently used" },
 { "insert", insert, METH_VARARGS, "Insert a token/doc pair" },
 { "flush", flush, METH_VARARGS, "Save the current changes to disk" },
 { "count", count, METH_VARARGS, "Returns how many times a token occurs" },
 { "fetch", fetch, METH_VARARGS, "Returns all documents associated with the given token" },
 { "intersect", intersect, METH_VARARGS, "Returns all documents associated with all of the given tokens" },
 { NULL, NULL, 0, NULL }
};

static struct PyModuleDef kvcpotmodule = {
    PyModuleDef_HEAD_INIT,
    "kvcpot",   /* name of module */
    NULL,
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    KVCpotMethods
};

PyMODINIT_FUNC
PyInit_kvcpot(void)
{
    PyObject *m;

    m = PyModule_Create(&kvcpotmodule);
    if (m == NULL) {
      return NULL;
    }

    return m;
}