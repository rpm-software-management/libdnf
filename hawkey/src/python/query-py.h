#ifndef QUERY_PY_H
#define QUERY_PY_H

#include "src/types.h"

/* additional Query constants only used in the bindings */
enum py_key_name_e {
    HY_PKG_DOWNGRADES = 100,
    HY_PKG_EMPTY,
    HY_PKG_LATEST,
    HY_PKG_UPGRADES
};

extern PyTypeObject query_Type;

#define queryObject_Check(o)	PyObject_TypeCheck(o, &query_Type)

HyQuery queryFromPyObject(PyObject *o);
int query_converter(PyObject *o, HyQuery *query_ptr);

#endif // QUERY_PY_H
