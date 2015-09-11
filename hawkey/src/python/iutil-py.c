/*
 * Copyright (C) 2012-2014 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "Python.h"

// hawkey
#include "src/advisory.h"
#include "src/advisorypkg.h"
#include "src/advisoryref.h"
#include "src/package_internal.h"
#include "src/packagelist.h"
#include "src/packageset_internal.h"
#include "src/reldep_internal.h"
#include "src/iutil.h"
#include "advisory-py.h"
#include "advisorypkg-py.h"
#include "advisoryref-py.h"
#include "iutil-py.h"
#include "package-py.h"
#include "reldep-py.h"
#include "sack-py.h"

#include "pycomp.h"

PyObject *
advisorylist_to_pylist(const HyAdvisoryList advisorylist, PyObject *sack)
{
    HyAdvisory cadvisory;
    PyObject *advisory;

    PyObject *list = PyList_New(0);
    if (list == NULL)
	return NULL;

    const int count = hy_advisorylist_count(advisorylist);
    for (int i = 0; i < count; ++i) {
	cadvisory = hy_advisorylist_get_clone(advisorylist,  i);
	advisory = advisoryToPyObject(cadvisory, sack);

	if (advisory == NULL)
	    goto fail;

	int rc = PyList_Append(list, advisory);
	Py_DECREF(advisory);
	if (rc == -1)
	    goto fail;
    }

    return list;
 fail:
    Py_DECREF(list);
    return NULL;
}

PyObject *
advisorypkglist_to_pylist(const HyAdvisoryPkgList advisorypkglist)
{
    HyAdvisoryPkg cadvisorypkg;
    PyObject *advisorypkg;

    PyObject *list = PyList_New(0);
    if (list == NULL)
	return NULL;

    const int count = hy_advisorypkglist_count(advisorypkglist);
    for (int i = 0; i < count; ++i) {
	cadvisorypkg = hy_advisorypkglist_get_clone(advisorypkglist,  i);
	advisorypkg = advisorypkgToPyObject(cadvisorypkg);

	if (advisorypkg == NULL) {
	    hy_advisorypkg_free(cadvisorypkg);
	    goto fail;
	}

	int rc = PyList_Append(list, advisorypkg);
	Py_DECREF(advisorypkg);
	if (rc == -1)
	    goto fail;
    }

    return list;
 fail:
    Py_DECREF(list);
    return NULL;
}

PyObject *
advisoryreflist_to_pylist(const HyAdvisoryRefList advisoryreflist, PyObject *sack)
{
    HyAdvisoryRef cadvisoryref;
    PyObject *advisoryref;

    PyObject *list = PyList_New(0);
    if (list == NULL)
	return NULL;

    const int count = hy_advisoryreflist_count(advisoryreflist);
    for (int i = 0; i < count; ++i) {
	cadvisoryref = hy_advisoryreflist_get_clone(advisoryreflist,  i);
	advisoryref = advisoryrefToPyObject(cadvisoryref, sack);

	if (advisoryref == NULL) {
	    hy_advisoryref_free(cadvisoryref);
	    goto fail;
	}

	int rc = PyList_Append(list, advisoryref);
	Py_DECREF(advisoryref);
	if (rc == -1)
	    goto fail;
    }

    return list;
 fail:
    Py_DECREF(list);
    return NULL;
}

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

PyObject *
packageset_to_pylist(HyPackageSet pset, PyObject *sack)
{
    PyObject *list = PyList_New(0);
    if (list == NULL)
	return NULL;

    const int count = hy_packageset_count(pset);
    Id id = -1;
    for (int i = 0; i < count; ++i) {
	id = packageset_get_pkgid(pset, i, id);
	PyObject *package = new_package(sack, id);
	if (package == NULL)
	    goto fail;

	int rc = PyList_Append(list, package);
	Py_DECREF(package);
	if (rc == -1)
	    goto fail;
    }

    return list;
 fail:
    Py_DECREF(list);
    return NULL;
}

HyPackageSet
pyseq_to_packageset(PyObject *obj, HySack sack)
{
    PyObject *sequence = PySequence_Fast(obj, "Expected a sequence.");
    if (sequence == NULL)
	return NULL;
    HyPackageSet pset = hy_packageset_create(sack);

    const unsigned count = PySequence_Size(sequence);
    for (int i = 0; i < count; ++i) {
	PyObject *item = PySequence_Fast_GET_ITEM(sequence, i);
	if (item == NULL)
	    goto fail;
	HyPackage pkg = packageFromPyObject(item);
	if (pkg == NULL)
	    goto fail;
	hy_packageset_add(pset, package_clone(pkg));
    }

    Py_DECREF(sequence);
    return pset;
 fail:
    hy_packageset_free(pset);
    Py_DECREF(sequence);
    return NULL;
}

HyReldepList
pyseq_to_reldeplist(PyObject *obj, HySack sack, int cmp_type)
{
    PyObject *sequence = PySequence_Fast(obj, "Expected a sequence.");
    if (sequence == NULL)
	return NULL;
    HyReldepList reldeplist = hy_reldeplist_create(sack);

    const unsigned count = PySequence_Size(sequence);
    for (int i = 0; i < count; ++i) {
	PyObject *item = PySequence_Fast_GET_ITEM(sequence, i);
	if (item == NULL)
	    goto fail;

    if (cmp_type == HY_GLOB) {
        HyReldepList g_reldeplist = NULL;
        const char *reldep_str = NULL;
        PyObject *tmp_py_str = NULL;

        reldep_str = pycomp_get_string(item, &tmp_py_str);
        if (reldep_str == NULL)
            goto fail;
        Py_XDECREF(tmp_py_str);

        g_reldeplist = reldeplist_from_str(sack, reldep_str);
        merge_reldeplists(reldeplist, g_reldeplist);
        hy_reldeplist_free(g_reldeplist);

    } else {
        HyReldep reldep = NULL;
        if reldepObject_Check(item)
            reldep = reldepFromPyObject(item);
        else
            reldep = reldep_from_pystr(item, sack);

        if (reldep != NULL)
            hy_reldeplist_add(reldeplist, reldep);
    }
    }

    Py_DECREF(sequence);
    return reldeplist;
 fail:
    hy_reldeplist_free(reldeplist);
    Py_DECREF(sequence);
    return NULL;
}

PyObject *
strlist_to_pylist(const char **slist)
{
    PyObject *list = PyList_New(0);
    if (list == NULL)
	return NULL;

    for (const char **iter = slist; *iter; ++iter) {
	PyObject *str = PyUnicode_FromString(*iter);
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
    PyObject *list = PyList_New(0);
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

HyReldep
reldep_from_pystr(PyObject *o, HySack sack)
{
    HyReldep reldep = NULL;
    const char *reldep_str = NULL;
    PyObject *tmp_py_str = NULL;

    reldep_str = pycomp_get_string(o, &tmp_py_str);
    if (reldep_str == NULL)
        return NULL;
    Py_XDECREF(tmp_py_str);

    reldep = reldep_from_str(sack, reldep_str);
    return reldep;
}
