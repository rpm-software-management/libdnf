#include <Python.h>

// hawkey
#include "src/sack.h"
#include "src/python/sack-py.h"
#include "tests/testsys.h"

static PyObject *
py_load_repo(PyObject *unused, PyObject *args)
{
    PyObject *sack = NULL;
    char *name = NULL, *path = NULL;
    int installed;

    if (!PyArg_ParseTuple(args, "Ossi", &sack, &name, &path, &installed))
	return NULL;

    Sack csack = sackFromPyObject(sack);
    if (csack == NULL) {
	PyErr_SetString(PyExc_TypeError, "Expected a Sack object.");
	return NULL;
    }
    if (load_repo(csack->pool, name, path, installed)) {
	PyErr_SetString(PyExc_IOError, "Can not load a testing repo.");
	return NULL;
    }
    Py_RETURN_NONE;
}

static struct PyMethodDef testmodule_methods[] = {
    {"load_repo",	(PyCFunction)py_load_repo,	 METH_VARARGS,
     NULL},
    {NULL}				/* sentinel */
};

PyMODINIT_FUNC
init_hawkey_test(void)
{
    Py_InitModule("_hawkey_test", testmodule_methods);
}
