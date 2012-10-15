#ifndef SELECTOR_PY_H
#define SELECTOR_PY_H

#include "src/types.h"

extern PyTypeObject selector_Type;

int selector_converter(PyObject *o, HySelector *sltr_ptr);

#endif // SELECTOR_PY_H
