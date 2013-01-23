#include <Python.h>

// hawkey
#include "src/goal.h"
#include "src/package.h"
#include "src/query.h"
#include "src/types.h"
#include "src/util.h"
#include "src/version.h"

// pyhawkey
#include "exception-py.h"
#include "goal-py.h"
#include "package-py.h"
#include "query-py.h"
#include "reldep-py.h"
#include "repo-py.h"
#include "sack-py.h"
#include "selector-py.h"

static PyObject *
py_chksum_name(PyObject *unused, PyObject *args)
{
    int i;
    const char *name;

    if (!PyArg_ParseTuple(args, "i", &i))
	return NULL;
    name = hy_chksum_name(i);
    if (name == NULL) {
	PyErr_Format(PyExc_ValueError, "unrecognized chksum type: %d", i);
	return NULL;
    }

    return PyString_FromString(name);
}

static PyObject *
py_chksum_type(PyObject *unused, PyObject *str_o)
{
    const char *str = PyString_AsString(str_o);
    if (str == NULL)
	return NULL;

    int type = hy_chksum_type(str);
    if (type == 0) {
	PyErr_Format(PyExc_ValueError, "unrecognized chksum type: %s", str);
	return NULL;
    }
    return PyInt_FromLong(type);
}

PyObject *
py_split_nevra(PyObject *unused, PyObject *nevra_o)
{
    const char *nevra = PyString_AsString(nevra_o);
    if (nevra == NULL)
	return NULL;
    long epoch;
    char *name, *version, *release, *arch;
    if (ret2e(hy_split_nevra(nevra, &name, &epoch, &version, &release, &arch),
	      "Failed parsing NEVRA."))
	return NULL;

    PyObject *ret = Py_BuildValue("slsss", name, epoch, version, release, arch);
    if (ret == NULL)
	return NULL;
    return ret;
}

static struct PyMethodDef hawkey_methods[] = {
    {"chksum_name",		(PyCFunction)py_chksum_name,
     METH_VARARGS,	NULL},
    {"chksum_type",		(PyCFunction)py_chksum_type,
     METH_O,		NULL},
    {"split_nevra",		(PyCFunction)py_split_nevra,
     METH_O,		NULL},
    {NULL}				/* sentinel */
};

PyMODINIT_FUNC
init_hawkey(void)
{
    PyObject *m = Py_InitModule("_hawkey", hawkey_methods);
    if (!m)
	return;
    /* exceptions */
    if (!init_exceptions())
	return;
    PyModule_AddObject(m, "Exception", HyExc_Exception);
    PyModule_AddObject(m, "ValueException", HyExc_Value);
    PyModule_AddObject(m, "QueryException", HyExc_Query);
    PyModule_AddObject(m, "ArchException", HyExc_Arch);
    PyModule_AddObject(m, "RuntimeException", HyExc_Runtime);
    PyModule_AddObject(m, "ValidationException", HyExc_Validation);

    /* _hawkey.Sack */
    if (PyType_Ready(&sack_Type) < 0)
	return;
    Py_INCREF(&sack_Type);
    PyModule_AddObject(m, "Sack", (PyObject *)&sack_Type);
    /* _hawkey.Goal */
    if (PyType_Ready(&goal_Type) < 0)
	return;
    Py_INCREF(&goal_Type);
    PyModule_AddObject(m, "Goal", (PyObject *)&goal_Type);
    /* _hawkey.Package */
    if (PyType_Ready(&package_Type) < 0)
	return;
    Py_INCREF(&package_Type);
    PyModule_AddObject(m, "Package", (PyObject *)&package_Type);
    /* _hawkey.Query */
    if (PyType_Ready(&query_Type) < 0)
	return;
    Py_INCREF(&query_Type);
    PyModule_AddObject(m, "Query", (PyObject *)&query_Type);
    /* _hawkey.Reldep */
    if (PyType_Ready(&reldep_Type) < 0)
	return;
    Py_INCREF(&reldep_Type);
    PyModule_AddObject(m, "Reldep", (PyObject *)&reldep_Type);
    /* _hawkey.Selector */
    if (PyType_Ready(&selector_Type) < 0)
	return;
    Py_INCREF(&selector_Type);
    PyModule_AddObject(m, "Selector", (PyObject *)&selector_Type);
    /* _hawkey.Repo */
    if (PyType_Ready(&repo_Type) < 0)
	return;
    Py_INCREF(&repo_Type);
    PyModule_AddObject(m, "Repo", (PyObject *)&repo_Type);

    PyModule_AddIntConstant(m, "VERSION_MAJOR", HY_VERSION_MAJOR);
    PyModule_AddIntConstant(m, "VERSION_MINOR", HY_VERSION_MINOR);
    PyModule_AddIntConstant(m, "VERSION_PATCH", HY_VERSION_PATCH);

    PyModule_AddStringConstant(m, "SYSTEM_REPO_NAME", HY_SYSTEM_REPO_NAME);
    PyModule_AddStringConstant(m, "CMDLINE_REPO_NAME", HY_CMDLINE_REPO_NAME);

    PyModule_AddIntConstant(m, "PKG", HY_PKG);
    PyModule_AddIntConstant(m, "PKG_DESCRIPTION", HY_PKG_DESCRIPTION);
    PyModule_AddIntConstant(m, "PKG_EPOCH", HY_PKG_EPOCH);
    PyModule_AddIntConstant(m, "PKG_NAME", HY_PKG_NAME);
    PyModule_AddIntConstant(m, "PKG_OBSOLETES", HY_PKG_OBSOLETES);
    PyModule_AddIntConstant(m, "PKG_URL", HY_PKG_URL);
    PyModule_AddIntConstant(m, "PKG_ARCH", HY_PKG_ARCH);
    PyModule_AddIntConstant(m, "PKG_EVR", HY_PKG_EVR);
    PyModule_AddIntConstant(m, "PKG_VERSION", HY_PKG_VERSION);
    PyModule_AddIntConstant(m, "PKG_LOCATION", HY_PKG_LOCATION);
    PyModule_AddIntConstant(m, "PKG_RELEASE", HY_PKG_RELEASE);
    PyModule_AddIntConstant(m, "PKG_SOURCERPM", HY_PKG_SOURCERPM);
    PyModule_AddIntConstant(m, "PKG_SUMMARY", HY_PKG_SUMMARY);
    PyModule_AddIntConstant(m, "PKG_FILE", HY_PKG_FILE);
    PyModule_AddIntConstant(m, "PKG_REPONAME", HY_PKG_REPONAME);
    PyModule_AddIntConstant(m, "PKG_PROVIDES", HY_PKG_PROVIDES);
    PyModule_AddIntConstant(m, "PKG_EMPTY", HY_PKG_EMPTY);
    PyModule_AddIntConstant(m, "PKG_LATEST", HY_PKG_LATEST);
    PyModule_AddIntConstant(m, "PKG_DOWNGRADES", HY_PKG_DOWNGRADES);
    PyModule_AddIntConstant(m, "PKG_UPGRADES", HY_PKG_UPGRADES);

    PyModule_AddIntConstant(m, "CHKSUM_MD5", HY_CHKSUM_MD5);
    PyModule_AddIntConstant(m, "CHKSUM_SHA1", HY_CHKSUM_SHA1);
    PyModule_AddIntConstant(m, "CHKSUM_SHA256", HY_CHKSUM_SHA256);

    PyModule_AddIntConstant(m, "ICASE", HY_ICASE);
    PyModule_AddIntConstant(m, "EQ", HY_EQ);
    PyModule_AddIntConstant(m, "LT", HY_LT);
    PyModule_AddIntConstant(m, "GT", HY_GT);
    PyModule_AddIntConstant(m, "NEQ", HY_NEQ);
    PyModule_AddIntConstant(m, "SUBSTR", HY_SUBSTR);
    PyModule_AddIntConstant(m, "GLOB", HY_GLOB);

    PyModule_AddIntConstant(m, "REASON_DEP", HY_REASON_DEP);
    PyModule_AddIntConstant(m, "REASON_USER", HY_REASON_USER);
}
