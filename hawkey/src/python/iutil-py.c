#include "Python.h"

// hawkey
#include "src/package_internal.h"
#include "src/packagelist.h"
#include "iutil-py.h"
#include "package-py.h"
#include "sack-py.h"

PyObject *
packagelist_to_pylist(HyPackageList plist, PyObject *sack)
{
    HyPackage cpkg;
    PyObject *list;
    PyObject *retval;

    list = PyList_New(0);
    if (list == NULL)
	return NULL;
    retval = list;

    int i;
    FOR_PACKAGELIST(cpkg, plist, i) {
	PyObject *package = new_package(sack, package_id(cpkg));
	if (package == NULL) {
	    retval = NULL;
	    break;
	}

	int rc = PyList_Append(list, package);
	Py_DECREF(package);
	if (rc == -1) {
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

HyPackageList
pylist_to_packagelist(PyObject *sequence)
{
    HyPackageList plist = hy_packagelist_create();

    assert(PySequence_Check(sequence));

    const unsigned count = PySequence_Size(sequence);
    for (int i = 0; i < count; ++i) {
	PyObject *item = PySequence_GetItem(sequence, i);
	if (item == NULL)
	    goto fail;
	HyPackage pkg = packageFromPyObject(item);
	Py_DECREF(item);
	if (pkg == NULL)
	    goto fail;
	hy_packagelist_push(plist, package_clone(pkg));
    }

    return plist;
 fail:
    hy_packagelist_free(plist);
    return NULL;
}

PyObject *
strlist_to_pylist(const char **slist)
{
    PyObject *list = PyList_New(0);
    if (list == NULL)
	return NULL;

    for (const char **iter = slist; *iter; ++iter) {
	PyObject *str = PyString_FromString(*iter);
	if (str == NULL)
	    goto err;
	int rc = PyList_Append(list, str);
	Py_DECREF(str);
	if (rc == -1)
	    goto err;
    }
    return list;

 err:
    Py_DECREF(list);
    return NULL;
}

