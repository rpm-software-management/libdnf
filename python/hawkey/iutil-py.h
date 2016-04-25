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

#include "hy-types.h"
#include "hif-sack.h"

#define TEST_COND(cond) \
    ((cond) ? Py_True : Py_False)

PyObject *advisorylist_to_pylist(const GPtrArray *advisorylist, PyObject *sack);
PyObject *advisorypkglist_to_pylist(const GPtrArray *advisorypkglist);
PyObject *advisoryreflist_to_pylist(const GPtrArray *advisoryreflist, PyObject *sack);
PyObject *packagelist_to_pylist(GPtrArray *plist, PyObject *sack);
PyObject *packageset_to_pylist(HifPackageSet *pset, PyObject *sack);
HifPackageSet *pyseq_to_packageset(PyObject *sequence, HifSack *sack);
HifReldepList *pyseq_to_reldeplist(PyObject *sequence, HifSack *sack, int cmp_type);
PyObject *strlist_to_pylist(const char **slist);
PyObject *reldeplist_to_pylist(HifReldepList *reldeplist, PyObject *sack);
HifReldep *reldep_from_pystr(PyObject *o, HifSack *sack);

#endif // IUTIL_PY_H
