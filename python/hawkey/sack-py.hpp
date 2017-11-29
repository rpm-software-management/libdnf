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

#ifndef SACK_PY_H
#define SACK_PY_H

// libsolv
#include <solv/pooltypes.h>

// hawkey
#include "hy-types.h"

extern PyTypeObject sack_Type;

#define sackObject_Check(o)        PyObject_TypeCheck(o, &sack_Type)

DnfSack *sackFromPyObject(PyObject *o);
int sack_converter(PyObject *o, DnfSack **sack_ptr);

PyObject *new_package(PyObject *sack, Id id);
gboolean set_logfile(const gchar *path, FILE *log_out);
const char *log_level_name(int level);

#endif // SACK_PY_H
