#ifndef EXCEPTION_PY_H
#define EXCEPTION_PY_H

extern PyObject *HyExc_Exception;
extern PyObject *HyExc_Value;
extern PyObject *HyExc_Query;
extern PyObject *HyExc_Arch;
extern PyObject *HyExc_Runtime;
extern PyObject *HyExc_Validation;

int init_exceptions(void);
int ret2e(int ret, const char *msg);

#endif // EXCEPTION_PY_H
