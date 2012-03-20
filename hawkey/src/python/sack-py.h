#ifndef SACK_PY_H
#define SACK_PY_H

#include "solv/pooltypes.h"
#include "src/types.h"

extern PyTypeObject sack_Type;

#define sackObject_Check(o)	PyObject_TypeCheck(o, &sack_Type)

HySack sackFromPyObject(PyObject *o);

PyObject *new_package(PyObject *sack, Id id);

#endif // SACK_PY_H
