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

#include <Python.h>

// hawkey
#include "src/package_internal.h"

// pyhawkey
#include "packagedelta-py.h"

typedef struct {
    PyObject_HEAD
    HyPackageDelta delta;
} _PackageDeltaObject;

PyObject *
packageDeltaToPyObject(HyPackageDelta delta)
{
    _PackageDeltaObject *self = (_PackageDeltaObject *)packageDelta_Type.
	tp_alloc(&packageDelta_Type, 0);
    self->delta = delta;
    return (PyObject *)self;
}

/* functions on the type */

static PyObject *
packageDelta_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
	return NULL;

    _PackageDeltaObject *self = (_PackageDeltaObject *)type->tp_alloc(type, 0);
    if (self)
	self->delta = delta_create();
    return (PyObject *) self;
}

static void
packageDelta_dealloc(_PackageDeltaObject *self)
{
    hy_packagedelta_free(self->delta);
    Py_TYPE(self)->tp_free(self);
}

/* getsetters */

static PyObject *
get_str(_PackageDeltaObject *self, void *closure)
{
    const char *(*func)(HyPackageDelta);
    const char *cstr;

    func = (const char *(*)(HyPackageDelta))closure;
    cstr = func(self->delta);
    return PyString_FromString(cstr);
}

static PyGetSetDef packageDelta_getsetters[] = {
    {"location", (getter)get_str, NULL, NULL,
     (void *)hy_packagedelta_get_location},
    {NULL}			/* sentinel */
};

/* type */

PyTypeObject packageDelta_Type = {
    PyObject_HEAD_INIT(NULL)
    0,				/*ob_size*/
    "_hawkey.PackageDelta",	/*tp_name*/
    sizeof(_PackageDeltaObject),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    (destructor) packageDelta_dealloc, /*tp_dealloc*/
    0,				/*tp_print*/
    0,				/*tp_getattr*/
    0,				/*tp_setattr*/
    0,				/*tp_compare*/
    0,				/*tp_repr*/
    0,				/*tp_as_number*/
    0,				/*tp_as_sequence*/
    0,				/*tp_as_mapping*/
    0,				/*tp_hash */
    0,				/*tp_call*/
    0,				/*tp_str*/
    0,				/*tp_getattro*/
    0,				/*tp_setattro*/
    0,				/*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,	/*tp_flags*/
    "PackageDelta object",	/* tp_doc */
    0,				/* tp_traverse */
    0,				/* tp_clear */
    0,				/* tp_richcompare */
    0,				/* tp_weaklistoffset */
    PyObject_SelfIter,		/* tp_iter */
    0,                         	/* tp_iternext */
    0,				/* tp_methods */
    0,				/* tp_members */
    packageDelta_getsetters,	/* tp_getset */
    0,				/* tp_base */
    0,				/* tp_dict */
    0,				/* tp_descr_get */
    0,				/* tp_descr_set */
    0,				/* tp_dictoffset */
    0,				/* tp_init */
    0,				/* tp_alloc */
    packageDelta_new,		/* tp_new */
    0,				/* tp_free */
    0,				/* tp_is_gc */
};
