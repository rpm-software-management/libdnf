#ifndef SACK_PY_H
#define SACK_PY_H

// libsolv
#include <solv/pooltypes.h>

// hawkey
#include "src/types.h"

extern PyTypeObject sack_Type;

#define sackObject_Check(o)	PyObject_TypeCheck(o, &sack_Type)

HySack sackFromPyObject(PyObject *o);
int sack_converter(PyObject *o, HySack *sack_ptr);

PyObject *new_package(PyObject *sack, Id id);

#endif // SACK_PY_H
