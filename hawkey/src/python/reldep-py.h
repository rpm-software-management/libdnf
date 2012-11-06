#ifndef RELDEP_PY_H
#define RELDEP_PY_H

#include "src/types.h"

PyObject *new_reldep(PyObject *sack, Id r_id);
extern PyTypeObject reldep_Type;

#endif // RELDEP_PY_H
