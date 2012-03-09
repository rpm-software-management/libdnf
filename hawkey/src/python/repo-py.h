#ifndef REPO_PY_H
#define REPO_PY_H

#include "src/frepo.h"

extern PyTypeObject repo_Type;

#define repoObject_Check(v)	((v)->ob_type == &repo_Type)

FRepo frepoFromPyObject(PyObject *o);

#endif // REPO_PY_H
