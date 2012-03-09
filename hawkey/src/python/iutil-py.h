#ifndef IUTIL_PY_H
#define IUTIL_PY_H

#include "src/packagelist.h"

PyObject *packagelist_to_pylist(PackageList plist, PyObject *sack);

#endif // IUTIL_PY_H
