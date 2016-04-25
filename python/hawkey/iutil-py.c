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

#include "hif-advisory.h"
#include "hif-advisorypkg.h"
#include "hif-advisoryref.h"
#include "hy-package-private.h"
#include "hy-packageset-private.h"
#include "hif-reldep-private.h"
#include "hif-reldep-list-private.h"
#include "hy-iutil.h"
#include "advisory-py.h"
#include "advisorypkg-py.h"
#include "advisoryref-py.h"
#include "iutil-py.h"
#include "package-py.h"
#include "reldep-py.h"
#include "sack-py.h"
#include "pycomp.h"

PyObject *
advisorylist_to_pylist(const GPtrArray *advisorylist, PyObject *sack)
{
    HifAdvisory *cadvisory;
    PyObject *advisory;

    PyObject *list = PyList_New(0);
    if (list == NULL)
        return NULL;

    for (unsigned int i = 0; i < advisorylist->len; ++i) {
        cadvisory = g_object_ref(g_ptr_array_index(advisorylist, i));
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
advisorypkglist_to_pylist(const GPtrArray *advisorypkglist)
{
    HifAdvisoryPkg *cadvisorypkg;
    PyObject *advisorypkg;

    PyObject *list = PyList_New(0);
    if (list == NULL)
        return NULL;

    for (unsigned int i = 0; i < advisorypkglist->len; ++i) {
        cadvisorypkg = g_object_ref(g_ptr_array_index(advisorypkglist,  i));
        advisorypkg = advisorypkgToPyObject(cadvisorypkg);
        if (advisorypkg == NULL)
            goto fail;

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
advisoryreflist_to_pylist(const GPtrArray *advisoryreflist, PyObject *sack)
{
    HifAdvisoryRef *cadvisoryref;
    PyObject *advisoryref;

    PyObject *list = PyList_New(0);
    if (list == NULL)
        return NULL;

    for (unsigned int i = 0; i < advisoryreflist->len; ++i) {
        cadvisoryref = g_object_ref(g_ptr_array_index(advisoryreflist,  i));
        advisoryref = advisoryrefToPyObject(cadvisoryref, sack);
        if (advisoryref == NULL)
            goto fail;

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
packagelist_to_pylist(GPtrArray *plist, PyObject *sack)
{
    HifPackage *cpkg;
    PyObject *list;
    PyObject *retval;

    list = PyList_New(0);
    if (list == NULL)
        return NULL;
    retval = list;

    for (unsigned int i = 0; i < plist->len; i++) {
        cpkg = g_ptr_array_index (plist, i);
        PyObject *package = new_package(sack, hif_package_get_id(cpkg));
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
packageset_to_pylist(HifPackageSet *pset, PyObject *sack)
{
    PyObject *list = PyList_New(0);
    if (list == NULL)
        return NULL;

    const int count = hif_packageset_count(pset);
    Id id = -1;
    for (int i = 0; i < count; ++i) {
        id = hif_packageset_get_pkgid(pset, i, id);
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

HifPackageSet *
pyseq_to_packageset(PyObject *obj, HifSack *sack)
{
    PyObject *sequence = PySequence_Fast(obj, "Expected a sequence.");
    if (sequence == NULL)
        return NULL;
    HifPackageSet *pset = hif_packageset_new(sack);

    const unsigned count = PySequence_Size(sequence);
    for (unsigned int i = 0; i < count; ++i) {
        PyObject *item = PySequence_Fast_GET_ITEM(sequence, i);
        if (item == NULL)
            goto fail;
        HifPackage *pkg = packageFromPyObject(item);
        if (pkg == NULL)
            goto fail;
        hif_packageset_add(pset, pkg);
    }

    Py_DECREF(sequence);
    return pset;
 fail:
    g_object_unref(pset);
    Py_DECREF(sequence);
    return NULL;
}

HifReldepList *
pyseq_to_reldeplist(PyObject *obj, HifSack *sack, int cmp_type)
{
    PyObject *sequence = PySequence_Fast(obj, "Expected a sequence.");
    if (sequence == NULL)
        return NULL;
    HifReldepList *reldeplist = hif_reldep_list_new (sack);

    const unsigned count = PySequence_Size(sequence);
    for (unsigned int i = 0; i < count; ++i) {
        PyObject *item = PySequence_Fast_GET_ITEM(sequence, i);
        if (item == NULL)
            goto fail;

    if (cmp_type == HY_GLOB) {
        HifReldepList *g_reldeplist = NULL;
        const char *reldep_str = NULL;
        PyObject *tmp_py_str = NULL;

        reldep_str = pycomp_get_string(item, &tmp_py_str);
        if (reldep_str == NULL)
            goto fail;
        Py_XDECREF(tmp_py_str);

        g_reldeplist = reldeplist_from_str (sack, reldep_str);
        hif_reldep_list_extend (reldeplist, g_reldeplist);
        g_object_unref (g_reldeplist);

    } else {
        HifReldep *reldep = NULL;
        if reldepObject_Check(item)
            reldep = reldepFromPyObject(item);
        else
            reldep = reldep_from_pystr(item, sack);

        if (reldep != NULL)
            hif_reldep_list_add (reldeplist, reldep);
    }
    }

    Py_DECREF(sequence);
    return reldeplist;
 fail:
    g_object_unref (reldeplist);
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
reldeplist_to_pylist(HifReldepList *reldeplist, PyObject *sack)
{
    PyObject *list = PyList_New(0);
    if (list == NULL)
        return NULL;

    const int count = hif_reldep_list_count (reldeplist);
    for (int i = 0; i < count; ++i) {
        HifReldep *creldep = hif_reldep_list_index (reldeplist,  i);
        PyObject *reldep = new_reldep(sack, hif_reldep_get_id (creldep));

        g_object_unref (creldep);
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

HifReldep *
reldep_from_pystr(PyObject *o, HifSack *sack)
{
    HifReldep *reldep = NULL;
    const char *reldep_str = NULL;
    PyObject *tmp_py_str = NULL;

    reldep_str = pycomp_get_string(o, &tmp_py_str);
    if (reldep_str == NULL)
        return NULL;
    Py_XDECREF(tmp_py_str);

    reldep = reldep_from_str(sack, reldep_str);
    return reldep;
}
