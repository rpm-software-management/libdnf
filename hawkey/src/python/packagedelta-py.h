#ifndef PACKAGEDELTA_PY_H
#define PACKAGEDELTA_PY_H

#include "src/types.h"

extern PyTypeObject packageDelta_Type;

PyObject *packageDeltaToPyObject(HyPackageDelta delta);

#endif // PACKAGEDELTA_PY_H
