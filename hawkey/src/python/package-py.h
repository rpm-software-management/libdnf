#ifndef PACKAGE_PY_H
#define PACKAGE_PY_H

#include "src/package.h"

extern PyTypeObject package_Type;

Package packageFromPyObject(PyObject *o);

#endif // PACKAGE_PY_H