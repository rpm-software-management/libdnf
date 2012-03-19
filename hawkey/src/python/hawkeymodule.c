#include <Python.h>

// hawkey
#include "src/query.h"

// pyhawkey
#include "goal-py.h"
#include "sack-py.h"
#include "package-py.h"
#include "query-py.h"
#include "repo-py.h"

PyMODINIT_FUNC
init_hawkey(void)
{
    PyObject *m = Py_InitModule("_hawkey", NULL);
    if (!m)
	return;
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
    /* _hawkey.Repo */
    if (PyType_Ready(&repo_Type) < 0)
	return;
    Py_INCREF(&repo_Type);
    PyModule_AddObject(m, "Repo", (PyObject *)&repo_Type);

    PyModule_AddStringConstant(m, "SYSTEM_REPO_NAME", SYSTEM_REPO_NAME);
    PyModule_AddStringConstant(m, "CMDLINE_REPO_NAME", CMDLINE_REPO_NAME);

    PyModule_AddIntConstant(m, "KN_PKG_NAME", KN_PKG_NAME);
    PyModule_AddIntConstant(m, "KN_PKG_SUMMARY", KN_PKG_SUMMARY);
    PyModule_AddIntConstant(m, "KN_PKG_REPO", KN_PKG_REPO);
    PyModule_AddIntConstant(m, "KN_PKG_PROVIDES", KN_PKG_PROVIDES);
    PyModule_AddIntConstant(m, "KN_PKG_LATEST", KN_PKG_LATEST);
    PyModule_AddIntConstant(m, "KN_PKG_UPDATES", KN_PKG_UPDATES);
    PyModule_AddIntConstant(m, "KN_PKG_OBSOLETING", KN_PKG_OBSOLETING);

    PyModule_AddIntConstant(m, "FT_EQ", FT_EQ);
    PyModule_AddIntConstant(m, "FT_LT", FT_LT);
    PyModule_AddIntConstant(m, "FT_GT", FT_GT);
    PyModule_AddIntConstant(m, "FT_NEQ", FT_NEQ);
    PyModule_AddIntConstant(m, "FT_SUBSTR", FT_SUBSTR);
}
