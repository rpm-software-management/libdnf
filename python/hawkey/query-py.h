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

#ifndef QUERY_PY_H
#define QUERY_PY_H

#include "hy-types.h"

/* additional Query constants only used in the bindings */
enum py_key_name_e {
    HY_PKG_DOWNGRADABLE = 100,
    HY_PKG_DOWNGRADES,
    HY_PKG_EMPTY,
    HY_PKG_LATEST_PER_ARCH,
    HY_PKG_LATEST,
    HY_PKG_UPGRADABLE,
    HY_PKG_UPGRADES
};

extern PyTypeObject query_Type;

#define queryObject_Check(o)        PyObject_TypeCheck(o, &query_Type)

HyQuery queryFromPyObject(PyObject *o);
PyObject *queryToPyObject(HyQuery query, PyObject *sack, PyTypeObject *custom_object_type);
int query_converter(PyObject *o, HyQuery *query_ptr);
gboolean filter_internal(HyQuery query, HySelector sltr, PyObject *sack, PyObject *args, PyObject *kwds);

#endif // QUERY_PY_H
