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
#include <vector>

#include "dnf-advisory.h"
#include "dnf-advisorypkg.h"
#include "dnf-advisoryref.h"
#include "hy-packageset.h"
#include "dnf-reldep.h"
#include "dnf-reldep-list.h"
#include "hy-iutil.h"
#include "advisory-py.hpp"
#include "advisorypkg-py.hpp"
#include "advisoryref-py.hpp"
#include "iutil-py.hpp"
#include "package-py.hpp"
#include "query-py.hpp"
#include "reldep-py.hpp"
#include "sack-py.hpp"
#include "pycomp.hpp"
#include "sack/advisorypkg.hpp"
#include "sack/packageset.hpp"
#include "sack/query.hpp"

#include "../../libdnf/repo/solvable/Dependency.hpp"
#include "libdnf/repo/solvable/DependencyContainer.hpp"

PyObject *
advisorylist_to_pylist(const GPtrArray *advisorylist, PyObject *sack)
{
    auto list = PyList_New(0);
    if (list == NULL)
        return NULL;

    for (unsigned int i = 0; i < advisorylist->len; ++i) {
        auto cadvisory =
            static_cast<DnfAdvisory *>(g_ptr_array_index(advisorylist, i));
        auto advisory = advisoryToPyObject(cadvisory, sack);

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
advisoryPkgVectorToPylist(const std::vector<libdnf::AdvisoryPkg> & advisorypkgs)
{
    auto list = PyList_New(0);
    if (list == NULL)
        return NULL;

    for (auto& advisorypkg : advisorypkgs) {
        auto pyAdvisoryPkg = advisorypkgToPyObject(new libdnf::AdvisoryPkg(advisorypkg));
        if (pyAdvisoryPkg == NULL)
            goto fail;
        int rc = PyList_Append(list, pyAdvisoryPkg);
        Py_DECREF(pyAdvisoryPkg);
        if (rc == -1)
            goto fail;
    }

    return list;

    fail:
        Py_DECREF(list);
        return NULL;
}

PyObject *
advisoryRefVectorToPylist(const std::vector<libdnf::AdvisoryRef> & advisoryRefs, PyObject *sack)
{
    auto list = PyList_New(0);
    if (list == NULL)
        return NULL;

    for (auto& advisoryRef : advisoryRefs) {
        auto pyAdvisoryRef = advisoryrefToPyObject(new libdnf::AdvisoryRef(advisoryRef), sack);
        if (pyAdvisoryRef == NULL)
            goto fail;
        int rc = PyList_Append(list, pyAdvisoryRef);
        Py_DECREF(pyAdvisoryRef);
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
    auto list = PyList_New(0);
    if (list == NULL)
        return NULL;
    auto retval = list;

    for (unsigned int i = 0; i < plist->len; i++) {
        auto cpkg = static_cast<DnfPackage *>(g_ptr_array_index(plist, i));
        PyObject *package = new_package(sack, dnf_package_get_id(cpkg));
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
packageset_to_pylist(const DnfPackageSet *pset, PyObject *sack)
{
    PyObject *list = PyList_New(0);
    if (list == NULL)
        return NULL;

    Id id = -1;
    while(true) {
        id = pset->next(id);
        if (id == -1)
            break;
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

std::unique_ptr<DnfPackageSet>
pyseq_to_packageset(PyObject *obj, DnfSack *sack)
{
    if (queryObject_Check(obj)) {
        HyQuery target = queryFromPyObject(obj);
        return std::unique_ptr<DnfPackageSet>(new libdnf::PackageSet(*target->runSet()));
    }

    PyObject * sequence = PySequence_Fast(obj, "Expected a sequence.");
    if (sequence == NULL)
        return NULL;
    auto pset = std::unique_ptr<DnfPackageSet>(new libdnf::PackageSet(sack));

    const unsigned count = PySequence_Size(sequence);
    for (unsigned int i = 0; i < count; ++i) {
        PyObject *item = PySequence_Fast_GET_ITEM(sequence, i);
        if (item == NULL)
            goto fail;
        DnfPackage *pkg = packageFromPyObject(item);
        if (pkg == NULL)
            goto fail;
        pset->set(pkg);
    }

    Py_DECREF(sequence);
    return pset;
 fail:
    Py_DECREF(sequence);
    return NULL;
}

DnfReldepList *
pyseq_to_reldeplist(PyObject *obj, DnfSack *sack, int cmp_type)
{
    PyObject *sequence = PySequence_Fast(obj, "Expected a sequence.");
    if (sequence == NULL)
        return NULL;
    DnfReldepList *reldeplist = dnf_reldep_list_new (sack);

    const unsigned count = PySequence_Size(sequence);
    for (unsigned int i = 0; i < count; ++i) {
        PyObject *item = PySequence_Fast_GET_ITEM(sequence, i);
        if (item == NULL)
            goto fail;

    if (cmp_type == HY_GLOB) {
        DnfReldepList *g_reldeplist = NULL;
        const char *reldep_str = NULL;
        PyObject *tmp_py_str = NULL;

        reldep_str = pycomp_get_string(item, &tmp_py_str);
        if (reldep_str == NULL)
            goto fail;

        Py_XDECREF(tmp_py_str);

        g_reldeplist = reldeplist_from_str (sack, reldep_str);
        if (g_reldeplist == NULL)
            goto fail;

        dnf_reldep_list_extend (reldeplist, g_reldeplist);

    } else {
        DnfReldep *reldep = NULL;
        if reldepObject_Check(item)
            reldep = reldepFromPyObject(item);
        else
            reldep = reldep_from_pystr(item, sack);

        if (reldep != NULL)
            dnf_reldep_list_add (reldeplist, reldep);
    }
    }

    Py_DECREF(sequence);
    return reldeplist;
 fail:
    delete reldeplist;
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
reldeplist_to_pylist(DnfReldepList *reldeplist, PyObject *sack)
{
    PyObject *list = PyList_New(0);
    if (list == NULL)
        return NULL;

    const int count = dnf_reldep_list_count (reldeplist);
    for (int i = 0; i < count; ++i) {
        DnfReldep *creldep = dnf_reldep_list_index (reldeplist,  i);
        PyObject *reldep = new_reldep(sack, dnf_reldep_get_id (creldep));

        dnf_reldep_free(creldep);
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

DnfReldep *
reldep_from_pystr(PyObject *o, DnfSack *sack)
{
    DnfReldep *reldep = NULL;
    const char *reldep_str = NULL;
    PyObject *tmp_py_str = NULL;

    reldep_str = pycomp_get_string(o, &tmp_py_str);
    if (reldep_str == NULL)
        return NULL;

    reldep = reldep_from_str(sack, reldep_str);
    Py_XDECREF(tmp_py_str);

    return reldep;
}
