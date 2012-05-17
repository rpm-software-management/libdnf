#include "Python.h"

// hawkey
#include "src/package_internal.h"
#include "src/packagelist.h"
#include "iutil-py.h"
#include "sack-py.h"

PyObject *
packagelist_to_pylist(HyPackageList plist, PyObject *sack)
{
    HyPackage cpkg;
    PyObject *package;
    PyObject *list;
    PyObject *retval;
    const int count = hy_packagelist_count(plist);

    list = PyList_New(0);
    if (list == NULL)
	return NULL;
    retval = list;
    for (int i = 0; i < count; ++i) {
	cpkg = hy_packagelist_get(plist, i);
	package = new_package(sack, package_id(cpkg));
	if (package == NULL) {
	    retval = NULL;
	    break;
	}
	if (PyList_Append(list, package) == -1) {
	    retval = NULL;
	    break;
	}
    }
    if (retval)
	return retval;
    /* return error */
    Py_DECREF(list);
    return NULL;
}
