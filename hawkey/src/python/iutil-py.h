#ifndef IUTIL_PY_H
#define IUTIL_PY_H

#include "src/types.h"

PyObject *packagelist_to_pylist(HyPackageList plist, PyObject *sack);
HyPackageList pyseq_to_packagelist(PyObject *sequence);
HyPackageSet pyseq_to_packageset(PyObject *sequence, HySack sack);
PyObject *strlist_to_pylist(const char **slist);
PyObject *reldeplist_to_pylist(const HyReldepList reldeplist, PyObject *sack);

#endif // IUTIL_PY_H
