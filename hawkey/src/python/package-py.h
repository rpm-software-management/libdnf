#ifndef PACKAGE_PY_H
#define PACKAGE_PY_H

#include "api/package.h"

extern PyTypeObject package_Type;

#define packageObject_Check(v)	((v)->ob_type == &package_Type)

PyObject *new_package(PyObject *sack, Package cpkg);
Package packageFromPyObject(PyObject *o);

#endif // PACKAGE_PY_H