#include <Python.h>
#include "exception-py.h"

PyObject *HyExc_Exception = NULL;
PyObject *HyExc_Query = NULL;

void
init_exceptions(void)
{
     HyExc_Exception = PyErr_NewException("_hawkey.Exception", NULL, NULL);
     Py_INCREF(HyExc_Exception);

     HyExc_Query = PyErr_NewException("_hawkey.QueryException", HyExc_Exception,
				      NULL);
     Py_INCREF(HyExc_Query);
}
