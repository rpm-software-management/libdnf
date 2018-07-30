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

#ifndef IUTIL_PY_H
#define IUTIL_PY_H

#include <vector>

#include "hy-types.h"
#include "dnf-sack.h"
#include "sack/advisorypkg.hpp"

#define TEST_COND(cond) \
    ((cond) ? Py_True : Py_False)

/**
* @brief Smart pointer to PyObject
*
* Owns and manages PyObject. Decrements the reference count for the PyObject
* (calls Py_XDECREF on it) when  UniquePtrPyObject goes out of scope.
*
* Implements subset of standard std::unique_ptr methods.
*/
class UniquePtrPyObject {
public:
    constexpr UniquePtrPyObject() noexcept : pyObj(NULL) {}
    explicit UniquePtrPyObject(PyObject * pyObj) noexcept : pyObj(pyObj) {}
    UniquePtrPyObject(UniquePtrPyObject && src) noexcept : pyObj(src.pyObj) { src.pyObj = NULL; }
    UniquePtrPyObject & operator =(UniquePtrPyObject && src) noexcept;
    explicit operator bool() const noexcept { return pyObj != NULL; }
    PyObject * get() const noexcept { return pyObj; }
    PyObject * release() noexcept;
    void reset(PyObject * pyObj = NULL) noexcept;
    ~UniquePtrPyObject();
private:
    PyObject * pyObj;
};

inline PyObject * UniquePtrPyObject::release() noexcept
{
    auto tmpObj = pyObj;
    pyObj = NULL;
    return tmpObj;
}

std::vector<const char *> pySequenceConverter(PyObject * pySequence);
PyObject *advisorylist_to_pylist(const GPtrArray *advisorylist, PyObject *sack);
PyObject *advisoryPkgVectorToPylist(const std::vector<libdnf::AdvisoryPkg> & advisorypkgs);
PyObject *advisoryRefVectorToPylist(const std::vector<libdnf::AdvisoryRef> & advisoryRefs,
                                    PyObject *sack);
PyObject *packagelist_to_pylist(GPtrArray *plist, PyObject *sack);
PyObject * packageset_to_pylist(const DnfPackageSet * pset, PyObject * sack);
std::unique_ptr<DnfPackageSet> pyseq_to_packageset(PyObject * sequence, DnfSack * sack);
std::unique_ptr<DnfReldepList> pyseq_to_reldeplist(PyObject *sequence, DnfSack *sack, int cmp_type);
PyObject *strlist_to_pylist(const char **slist);
PyObject *reldeplist_to_pylist(DnfReldepList *reldeplist, PyObject *sack);

#endif // IUTIL_PY_H
