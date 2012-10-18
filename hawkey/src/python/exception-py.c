#include <Python.h>

// hawkey
#include "src/errno.h"

// pyhawkey
#include "exception-py.h"

PyObject *HyExc_Exception = NULL;
PyObject *HyExc_Value = NULL;
PyObject *HyExc_Query = NULL;
PyObject *HyExc_Arch = NULL;
PyObject *HyExc_Runtime = NULL;
PyObject *HyExc_Validation = NULL;

int
init_exceptions(void)
{
     HyExc_Exception = PyErr_NewException("_hawkey.Exception", NULL, NULL);
     if (!HyExc_Exception)
	 return 0;
     Py_INCREF(HyExc_Exception);

     HyExc_Value = PyErr_NewException("_hawkey.ValueException", HyExc_Exception,
				      NULL);
     if (!HyExc_Value)
	 return 0;
     Py_INCREF(HyExc_Value);

     HyExc_Query = PyErr_NewException("_hawkey.QueryException", HyExc_Value,
				      NULL);
     if (!HyExc_Query)
	 return 0;
     Py_INCREF(HyExc_Query);

     HyExc_Arch = PyErr_NewException("_hawkey.ArchException", HyExc_Value,
				      NULL);
     if (!HyExc_Arch)
	 return 0;
     Py_INCREF(HyExc_Arch);

     HyExc_Runtime = PyErr_NewException("_hawkey.RuntimeException",
					HyExc_Exception, NULL);
     if (!HyExc_Runtime)
	 return 0;
     Py_INCREF(HyExc_Runtime);

     HyExc_Validation = PyErr_NewException("_hawkey.ValidationException",
					HyExc_Exception, NULL);
     if (!HyExc_Validation)
	 return 0;
     Py_INCREF(HyExc_Validation);

     return 1;
}

int
ret2e(int ret, const char *msg)
{
    PyObject *exctype = NULL;
    switch (ret) {
    case 0:
	return 0;
    case HY_E_FAILED:
	exctype = HyExc_Runtime;
	break;
    case HY_E_OP:
    case HY_E_SELECTOR:
	exctype = HyExc_Value;
	break;
    default:
	assert(0);
	return 1;
    }

    assert(exctype);
    PyErr_SetString(exctype, msg);
    return 1;
}
