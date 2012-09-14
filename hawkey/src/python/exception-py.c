#include <Python.h>
#include "exception-py.h"

PyObject *HyExc_Exception = NULL;
PyObject *HyExc_Query = NULL;
PyObject *HyExc_Runtime = NULL;

int
init_exceptions(void)
{
     HyExc_Exception = PyErr_NewException("_hawkey.Exception", NULL, NULL);
     if (!HyExc_Exception)
	 return 0;
     Py_INCREF(HyExc_Exception);

     HyExc_Query = PyErr_NewException("_hawkey.QueryException", HyExc_Exception,
				      NULL);
     if (!HyExc_Query)
	 return 0;
     Py_INCREF(HyExc_Query);

     HyExc_Runtime = PyErr_NewException("_hawkey.RuntimeException",
					HyExc_Exception, NULL);
     if (!HyExc_Runtime)
	 return 0;
     Py_INCREF(HyExc_Runtime);

     return 1;
}
