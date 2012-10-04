#ifndef IUTIL_PY_H
#define IUTIL_PY_H

#include "src/types.h"

PyObject *packagelist_to_pylist(HyPackageList plist, PyObject *sack);
HyPackageList pylist_to_packagelist(PyObject *list);
PyObject *strlist_to_pylist(const char **slist);

#endif // IUTIL_PY_H
