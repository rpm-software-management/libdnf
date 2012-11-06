#ifndef RELDEP_PY_H
#define RELDEP_PY_H

#include "src/types.h"

extern PyTypeObject reldep_Type;

#define reldepObject_Check(o)	PyObject_TypeCheck(o, &reldep_Type)

PyObject *new_reldep(PyObject *sack, Id r_id);
HyReldep reldepFromPyObject(PyObject *o);

#endif // RELDEP_PY_H
