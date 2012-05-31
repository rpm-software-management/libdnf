#ifndef REPO_PY_H
#define REPO_PY_H

#include "src/types.h"

extern PyTypeObject repo_Type;

#define repoObject_Check(v)	((v)->ob_type == &repo_Type)

HyRepo repoFromPyObject(PyObject *o);
PyObject *repoToPyObject(HyRepo repo);

#endif // REPO_PY_H
