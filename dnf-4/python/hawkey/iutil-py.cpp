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
#include <string>
#include <ctime>
#include <datetime.h>

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
#include "sack/changelog.hpp"
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
            static_cast<DnfAdvisory *>(g_steal_pointer(&g_ptr_array_index(advisorylist, i)));
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
changelogslist_to_pylist(const std::vector<libdnf::Changelog> & changelogslist)
{
    UniquePtrPyObject list(PyList_New(0));
    if (!list)
        return NULL;
    PyDateTime_IMPORT;

    for (auto & citem: changelogslist) {
        UniquePtrPyObject d(PyDict_New());
        if (!d)
            return NULL;
        UniquePtrPyObject author(PyUnicode_FromString(citem.getAuthor().c_str()));
        if (PyDict_SetItemString(d.get(), "author", author.get()) == -1)
            return NULL;
        UniquePtrPyObject description(PyUnicode_FromString(citem.getText().c_str()));
        if (PyDict_SetItemString(d.get(), "text", description.get()) == -1)
            return NULL;
        time_t itemts=citem.getTimestamp();
        struct tm ts;
        ts = *localtime(&itemts);
        UniquePtrPyObject timestamp(PyDate_FromDate(ts.tm_year+1900, ts.tm_mon+1, ts.tm_mday));
        if (PyDict_SetItemString(d.get(), "timestamp", timestamp.get()) == -1)
            return NULL;
        if (PyList_Append(list.get(), d.get()) == -1)
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

std::unique_ptr<libdnf::DependencyContainer>
pyseq_to_reldeplist(PyObject *obj, DnfSack *sack, int cmp_type)
{
    UniquePtrPyObject sequence(PySequence_Fast(obj, "Expected a sequence."));
    if (!sequence)
        return NULL;
    std::unique_ptr<libdnf::DependencyContainer> reldeplist(new libdnf::DependencyContainer(sack));

    const unsigned count = PySequence_Size(sequence.get());
    for (unsigned int i = 0; i < count; ++i) {
        PyObject *item = PySequence_Fast_GET_ITEM(sequence.get(), i);
        if (item == NULL)
            return NULL;
        if (reldepObject_Check(item)) {
            DnfReldep * reldep = reldepFromPyObject(item);
            if (reldep == NULL)
                return NULL;
            reldeplist->add(reldep);
        } else if (cmp_type == HY_GLOB) {

            PycompString reldep_str(item);
            if (!reldep_str.getCString())
                return NULL;

            if (!hy_is_glob_pattern(reldep_str.getCString())) {
                if (!reldeplist->addReldep(reldep_str.getCString()))
                    continue;
            } else {
                if (!reldeplist->addReldepWithGlob(reldep_str.getCString()))
                    continue;
            }

        } else {
            PycompString reldepStr(item);
            if (!reldepStr.getCString())
                return NULL;
            if (!reldeplist->addReldep(reldepStr.getCString()))
                continue;
        }
    }

    return reldeplist;
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
strCpplist_to_pylist(const std::vector<std::string> & cppList)
{
    UniquePtrPyObject list(PyList_New(0));
    if (!list)
        return NULL;
    for (auto & cStr:cppList) {
        UniquePtrPyObject str(PyUnicode_FromString(cStr.c_str()));
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

    const int count = reldeplist->count();
    for (int i = 0; i < count; ++i) {
        UniquePtrPyObject reldep(new_reldep(sack, reldeplist->getId(i)));

        if (!reldep)
            return NULL;

        int rc = PyList_Append(list.get(), reldep.get());
        if (rc == -1)
            return NULL;
    }

    return list.release();
}

std::vector<std::string>
pySequenceConverter(PyObject * pySequence)
{
    UniquePtrPyObject seq(PySequence_Fast(pySequence, "Expected a sequence."));
    if (!seq)
        throw std::runtime_error("Expected a sequence.");
    const unsigned count = PySequence_Size(seq.get());
    std::vector<std::string> output;
    output.reserve(count);
    for (unsigned int i = 0; i < count; ++i) {
        PyObject *item = PySequence_Fast_GET_ITEM(seq.get(), i);
        if (PyUnicode_Check(item) || PyString_Check(item)) {
            PycompString pycomStr(item);
            if (!pycomStr.getCString())
                throw std::runtime_error("Invalid value.");
            output.push_back(pycomStr.getCString());
        } else {
            PyErr_SetString(PyExc_TypeError, "Invalid value.");
            throw std::runtime_error("Invalid value.");
        }
    }
    return output;
}

PyObject *
problemRulesPyConverter(std::vector<std::vector<std::string>> & allProblems)
{
    UniquePtrPyObject list_output(PyList_New(0));
    if (!list_output)
        return NULL;
    for (auto & problemList: allProblems) {
        if (problemList.empty()) {
            PyErr_SetString(PyExc_ValueError, "Index out of range.");
            continue;
        }
        UniquePtrPyObject list(strCpplist_to_pylist(problemList));
        int rc = PyList_Append(list_output.get(), list.get());
        if (rc == -1)
            return NULL;
    }
    return list_output.release();
}
