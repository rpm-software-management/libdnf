#include <Python.h>

PyMODINIT_FUNC
init_hawkey_test(void)
{
    PyObject *m = Py_InitModule("_hawkey_test", NULL);
    if (!m)
	return;

    PyModule_AddStringConstant(m, "TESTING", "some situations");
}
