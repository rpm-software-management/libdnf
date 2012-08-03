#ifndef EXCEPTION_PY_H
#define EXCEPTION_PY_H

extern PyObject *HyExc_Exception;
extern PyObject *HyExc_Query;

void init_exceptions(void);

#endif // EXCEPTION_PY_H
