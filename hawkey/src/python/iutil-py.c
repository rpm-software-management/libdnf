#include "Python.h"

// hawkey
#include "src/package_internal.h"
#include "src/packagelist.h"
#include "src/packageset.h"
#include "src/reldep_internal.h"
#include "iutil-py.h"
#include "package-py.h"
#include "reldep-py.h"
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
pyseq_to_packagelist(PyObject *sequence)
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

HyPackageSet
pyseq_to_packageset(PyObject *sequence, HySack sack)
{
    assert(PySequence_Check(sequence));
    HyPackageSet pset = hy_packageset_create(sack);

    const unsigned count = PySequence_Size(sequence);
    for (int i = 0; i < count; ++i) {
	PyObject *item = PySequence_GetItem(sequence, i);
	if (item == NULL)
	    goto fail;
	HyPackage pkg = packageFromPyObject(item);
	Py_DECREF(item);
	if (pkg == NULL)
	    goto fail;
	hy_packageset_add(pset, package_clone(pkg));
    }

    return pset;
 fail:
    hy_packageset_free(pset);
    return NULL;
}

HyReldepList
pyseq_to_reldeplist(PyObject *sequence, HySack sack)
{
    assert(PySequence_Check(sequence));
    HyReldepList reldeplist = hy_reldeplist_create(sack);

    const unsigned count = PySequence_Size(sequence);
    for (int i = 0; i < count; ++i) {
	PyObject *item = PySequence_GetItem(sequence, i);
	if (item == NULL)
	    goto fail;
	HyReldep reldep = reldepFromPyObject(item);
	Py_DECREF(item);
	if (reldep == NULL)
	    goto fail;
	hy_reldeplist_add(reldeplist, reldep);
    }

    return reldeplist;
 fail:
    hy_reldeplist_free(reldeplist);
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

PyObject *
reldeplist_to_pylist(const HyReldepList reldeplist, PyObject *sack)
{
    PyObject *list;
    list = PyList_New(0);
    if (list == NULL)
	return NULL;

    const int count = hy_reldeplist_count(reldeplist);
    for (int i = 0; i < count; ++i) {
	HyReldep creldep = hy_reldeplist_get_clone(reldeplist,  i);
	PyObject *reldep = new_reldep(sack, reldep_id(creldep));

	hy_reldep_free(creldep);
	if (reldep == NULL)
	    goto fail;

	int rc = PyList_Append(list, reldep);
	Py_DECREF(reldep);
	if (rc == -1)
	    goto fail;
    }

    return list;
 fail:
    Py_DECREF(list);
    return NULL;
}
