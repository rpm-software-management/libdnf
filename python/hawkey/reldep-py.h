/*
 * Copyright (C) 2012-2013 Red Hat, Inc.
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

#ifndef RELDEP_PY_H
#define RELDEP_PY_H

// libsolv
#include <solv/pooltypes.h>

// hawkey
#include "hy-types.h"

extern PyTypeObject reldep_Type;

#define reldepObject_Check(o)        PyObject_TypeCheck(o, &reldep_Type)

PyObject *new_reldep(PyObject *sack, Id r_id);
DnfReldep *reldepFromPyObject(PyObject *o);
PyObject *reldepToPyObject(DnfReldep *reldep);

#endif // RELDEP_PY_H
