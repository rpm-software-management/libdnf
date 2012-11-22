#include <Python.h>

// hawkey
#include "src/sack_internal.h"
#include "src/python/repo-py.h"
#include "src/python/sack-py.h"
#include "tests/testshared.h"

static PyObject *
py_load_repo(PyObject *unused, PyObject *args)
{
    PyObject *sack = NULL;
    char *name = NULL, *path = NULL;
    int installed;

    if (!PyArg_ParseTuple(args, "Ossi", &sack, &name, &path, &installed))
	return NULL;

    HySack csack = sackFromPyObject(sack);
    if (csack == NULL) {
	PyErr_SetString(PyExc_TypeError, "Expected a HySack object.");
	return NULL;
    }
    if (load_repo(sack_pool(csack), name, path, installed)) {
	PyErr_SetString(PyExc_IOError, "Can not load a testing repo.");
	return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
py_glob_for_repofiles(PyObject *unused, PyObject *args)
{
    const char *repo_name, *path;
    HySack sack;

    if (!PyArg_ParseTuple(args, "O&ss",
			  sack_converter, &sack, &repo_name, &path))
	return NULL;
    HyRepo repo = glob_for_repofiles(sack_pool(sack), repo_name, path);
    return repoToPyObject(repo);
}

static struct PyMethodDef testmodule_methods[] = {
    {"load_repo",		(PyCFunction)py_load_repo,
     METH_VARARGS, NULL},
    {"glob_for_repofiles",	(PyCFunction)py_glob_for_repofiles,
     METH_VARARGS, NULL},
    {NULL}				/* sentinel */
};

PyMODINIT_FUNC
init_hawkey_test(void)
{
    PyObject *m = Py_InitModule("_hawkey_test", testmodule_methods);
    if (!m)
	return;
    PyModule_AddIntConstant(m, "EXPECT_SYSTEM_NSOLVABLES",
			    TEST_EXPECT_SYSTEM_NSOLVABLES);
    PyModule_AddIntConstant(m, "EXPECT_MAIN_NSOLVABLES",
			    TEST_EXPECT_MAIN_NSOLVABLES);
    PyModule_AddIntConstant(m, "EXPECT_UPDATES_NSOLVABLES",
			    TEST_EXPECT_UPDATES_NSOLVABLES);
    PyModule_AddIntConstant(m, "EXPECT_YUM_NSOLVABLES",
			    TEST_EXPECT_YUM_NSOLVABLES);
    PyModule_AddStringConstant(m, "FIXED_ARCH", TEST_FIXED_ARCH);
    PyModule_AddStringConstant(m, "UNITTEST_DIR", UNITTEST_DIR);
    PyModule_AddStringConstant(m, "YUM_DIR_SUFFIX", YUM_DIR_SUFFIX);
}
