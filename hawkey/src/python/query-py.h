#ifndef QUERY_PY_H
#define QUERY_PY_H

#include "src/types.h"

extern PyTypeObject query_Type;

#define queryObject_Check(o)	PyObject_TypeCheck(o, &query_Type)

int query_converter(PyObject *o, HyQuery *query_ptr);

#endif // QUERY_PY_H
