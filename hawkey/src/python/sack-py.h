#ifndef SACK_PY_H
#define SACK_PY_H

#include "src/sack.h"

extern PyTypeObject sack_Type;

#define sackObject_Check(o)	PyObject_TypeCheck(o, &sack_Type)

Sack sackFromPyObject(PyObject *o);

#endif // SACK_PY_H
