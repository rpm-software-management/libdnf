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
#include "hy-util-private.hpp"
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

UniquePtrPyObject & UniquePtrPyObject::operator =(UniquePtrPyObject && src) noexcept
{
    if (this == &src)
        return *this;
    Py_XDECREF(pyObj);
    pyObj = src.pyObj;
    src.pyObj = NULL;
    return *this;
}

void UniquePtrPyObject::reset(PyObject * pyObj) noexcept
{
    Py_XDECREF(this->pyObj);
    this->pyObj = pyObj;
}

UniquePtrPyObject::~UniquePtrPyObject()
{
    Py_XDECREF(pyObj);
}

PyObject *
advisorylist_to_pylist(const GPtrArray *advisorylist, PyObject *sack)
{
    UniquePtrPyObject list(PyList_New(0));
    if (!list)
        return NULL;

    for (unsigned int i = 0; i < advisorylist->len; ++i) {
        auto cadvisory =
            static_cast<DnfAdvisory *>(g_ptr_array_index(advisorylist, i));
        UniquePtrPyObject advisory(advisoryToPyObject(cadvisory, sack));

        if (!advisory)
            return NULL;

        int rc = PyList_Append(list.get(), advisory.get());
        if (rc == -1)
            return NULL;
    }

    return list.release();
}

PyObject *
advisoryPkgVectorToPylist(const std::vector<libdnf::AdvisoryPkg> & advisorypkgs)
{
    UniquePtrPyObject list(PyList_New(0));
    if (!list)
        return NULL;

    for (auto& advisorypkg : advisorypkgs) {
        UniquePtrPyObject pyAdvisoryPkg(advisorypkgToPyObject(new libdnf::AdvisoryPkg(advisorypkg)));
        if (!pyAdvisoryPkg)
            return NULL;
        int rc = PyList_Append(list.get(), pyAdvisoryPkg.get());
        if (rc == -1)
            return NULL;
    }

    return list.release();
}

PyObject *
advisoryRefVectorToPylist(const std::vector<libdnf::AdvisoryRef> & advisoryRefs, PyObject *sack)
{
    UniquePtrPyObject list(PyList_New(0));
    if (!list)
        return NULL;

    for (auto& advisoryRef : advisoryRefs) {
        UniquePtrPyObject pyAdvisoryRef(advisoryrefToPyObject(new libdnf::AdvisoryRef(advisoryRef), sack));
        if (!pyAdvisoryRef)
            return NULL;
        int rc = PyList_Append(list.get(), pyAdvisoryRef.get());
        if (rc == -1)
            return NULL;
    }

    return list.release();
}

PyObject *
packagelist_to_pylist(GPtrArray *plist, PyObject *sack)
{
    UniquePtrPyObject list(PyList_New(0));
    if (!list)
        return NULL;

    for (unsigned int i = 0; i < plist->len; i++) {
        auto cpkg = static_cast<DnfPackage *>(g_ptr_array_index(plist, i));
        UniquePtrPyObject package(new_package(sack, dnf_package_get_id(cpkg)));
        if (!package)
            return NULL;

        int rc = PyList_Append(list.get(), package.get());
        if (rc == -1)
            return NULL;
    }
    return list.release();
}

PyObject *
packageset_to_pylist(const DnfPackageSet *pset, PyObject *sack)
{
    UniquePtrPyObject list(PyList_New(0));
    if (!list)
        return NULL;

    Id id = -1;
    while(true) {
        id = pset->next(id);
        if (id == -1)
            break;
        UniquePtrPyObject package(new_package(sack, id));
        if (!package)
            return NULL;

        int rc = PyList_Append(list.get(), package.get());
        if (rc == -1)
            return NULL;
    }

    return list.release();
}

std::unique_ptr<DnfPackageSet>
pyseq_to_packageset(PyObject *obj, DnfSack *sack)
{
    if (queryObject_Check(obj)) {
        HyQuery target = queryFromPyObject(obj);
        return std::unique_ptr<DnfPackageSet>(new libdnf::PackageSet(*target->runSet()));
    }

    UniquePtrPyObject sequence(PySequence_Fast(obj, "Expected a sequence."));
    if (!sequence)
        return NULL;
    std::unique_ptr<DnfPackageSet> pset(new libdnf::PackageSet(sack));

    const unsigned count = PySequence_Size(sequence.get());
    for (unsigned int i = 0; i < count; ++i) {
        PyObject *item = PySequence_Fast_GET_ITEM(sequence.get(), i);
        if (item == NULL)
            return NULL;
        DnfPackage *pkg = packageFromPyObject(item);
        if (pkg == NULL)
            return NULL;
        pset->set(pkg);
    }

    return pset;
}

DnfReldepList *
pyseq_to_reldeplist(PyObject *obj, DnfSack *sack, int cmp_type)
{
    UniquePtrPyObject sequence(PySequence_Fast(obj, "Expected a sequence."));
    if (!sequence)
        return NULL;
    DnfReldepList *reldeplist = dnf_reldep_list_new (sack);

    const unsigned count = PySequence_Size(sequence.get());
    for (unsigned int i = 0; i < count; ++i) {
        PyObject *item = PySequence_Fast_GET_ITEM(sequence.get(), i);
        if (item == NULL)
            goto fail;
        if reldepObject_Check(item) {
            DnfReldep * reldep = reldepFromPyObject(item);
            if (reldep == NULL)
                goto fail;
            dnf_reldep_list_add(reldeplist, reldep);
        } else if (cmp_type == HY_GLOB) {

            PycompString reldep_str(item);
            if (!reldep_str.getCString())
                goto fail;

            if (!hy_is_glob_pattern(reldep_str.getCString())) {
                try {
                    Dependency reldep(sack, reldep_str.getCString());
                    dnf_reldep_list_add(reldeplist, &reldep);
                }
                catch (...)
                {
                    goto fail;
                }
            } else {
                DnfReldepList * g_reldeplist = reldeplist_from_str(sack, reldep_str.getCString());
                if (g_reldeplist == NULL)
                    goto fail;
                dnf_reldep_list_extend(reldeplist, g_reldeplist);
                delete g_reldeplist;
            }

        } else {
            DnfReldep * reldep = reldep_from_pystr(item, sack);
            if (reldep == NULL)
                goto fail;
            dnf_reldep_list_add(reldeplist, reldep);
            delete reldep;
        }
    }

    return reldeplist;
 fail:
    delete reldeplist;
    return NULL;
}

PyObject *
strlist_to_pylist(const char **slist)
{
    UniquePtrPyObject list(PyList_New(0));
    if (!list)
        return NULL;

    for (const char **iter = slist; *iter; ++iter) {
        UniquePtrPyObject str(PyUnicode_FromString(*iter));
        if (!str)
            return NULL;
        int rc = PyList_Append(list.get(), str.get());
        if (rc == -1)
            return NULL;
    }
    return list.release();
}

PyObject *
reldeplist_to_pylist(DnfReldepList *reldeplist, PyObject *sack)
{
    UniquePtrPyObject list(PyList_New(0));
    if (!list)
        return NULL;

    const int count = dnf_reldep_list_count (reldeplist);
    for (int i = 0; i < count; ++i) {
        DnfReldep *creldep = dnf_reldep_list_index (reldeplist,  i);
        UniquePtrPyObject reldep(new_reldep(sack, dnf_reldep_get_id (creldep)));

        dnf_reldep_free(creldep);
        if (!reldep)
            return NULL;

        int rc = PyList_Append(list.get(), reldep.get());
        if (rc == -1)
            return NULL;
    }

    return list.release();
}

DnfReldep *
reldep_from_pystr(PyObject *o, DnfSack *sack)
{
    PycompString reldep_str(o);
    if (!reldep_str.getCString())
        return NULL;

    return new Dependency(sack, reldep_str.getCString());
}
