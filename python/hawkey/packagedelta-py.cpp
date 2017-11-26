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
#include "hy-iutil.h"

// pyhawkey
#include "packagedelta-py.hpp"

#include "pycomp.hpp"

typedef struct {
    PyObject_HEAD
    DnfPackageDelta *delta;
} _PackageDeltaObject;

PyObject *
packageDeltaToPyObject(DnfPackageDelta *delta)
{
    _PackageDeltaObject *self = PyObject_New(_PackageDeltaObject, &packageDelta_Type);
    self->delta = delta;
    return (PyObject *)self;
}

/* functions on the type */

static void
packageDelta_dealloc(_PackageDeltaObject *self)
{
    g_object_unref(self->delta);
    Py_TYPE(self)->tp_free(self);
}

/* getsetters */

static PyObject *
get_str(_PackageDeltaObject *self, void *closure)
{
    const char *(*func)(DnfPackageDelta *);
    const char *cstr;

    func = (const char *(*)(DnfPackageDelta *))closure;
    cstr = func(self->delta);
    if (cstr == NULL)
        Py_RETURN_NONE;
    return PyString_FromString(cstr);
}

static PyObject *
get_num(_PackageDeltaObject *self, void *closure)
{
    guint64 (*func)(DnfPackageDelta *);
    func = (guint64 (*)(DnfPackageDelta *))closure;
    return PyLong_FromUnsignedLongLong(func(self->delta));
}

static PyObject *
get_chksum(_PackageDeltaObject *self, void *closure)
{
    HyChecksum *(*func)(DnfPackageDelta *, int *);
    int type;
    HyChecksum *cs;

    func = (HyChecksum *(*)(DnfPackageDelta *, int *))closure;
    cs = func(self->delta, &type);
    if (cs == 0) {
        PyErr_SetString(PyExc_AttributeError, "No such checksum.");
        return NULL;
    }

    PyObject *res;
    int checksum_length = checksum_type2length(type);

#if PY_MAJOR_VERSION < 3
    res = Py_BuildValue("is#", type, cs, checksum_length);
#else
    res = Py_BuildValue("iy#", type, cs, checksum_length);
#endif

    return res;
}

static PyGetSetDef packageDelta_getsetters[] = {
    {(char*) "location", (getter)get_str, NULL, NULL,
     (void *)dnf_packagedelta_get_location},
    {(char*) "baseurl", (getter)get_str, NULL, NULL,
     (void *)dnf_packagedelta_get_baseurl},
    {(char*) "downloadsize", (getter)get_num, NULL, NULL,
     (void *)dnf_packagedelta_get_downloadsize},
    {(char*) "chksum", (getter)get_chksum, NULL, NULL,
    (void *)dnf_packagedelta_get_chksum},
    {NULL}                        /* sentinel */
};

/* type */

PyTypeObject packageDelta_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hawkey.PackageDelta",        /*tp_name*/
    sizeof(_PackageDeltaObject),        /*tp_basicsize*/
    0,                                /*tp_itemsize*/
    (destructor) packageDelta_dealloc, /*tp_dealloc*/
    0,                                /*tp_print*/
    0,                                /*tp_getattr*/
    0,                                /*tp_setattr*/
    0,                                /*tp_compare*/
    0,                                /*tp_repr*/
    0,                                /*tp_as_number*/
    0,                                /*tp_as_sequence*/
    0,                                /*tp_as_mapping*/
    0,                                /*tp_hash */
    0,                                /*tp_call*/
    0,                                /*tp_str*/
    0,                                /*tp_getattro*/
    0,                                /*tp_setattro*/
    0,                                /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "PackageDelta object",        /* tp_doc */
    0,                                /* tp_traverse */
    0,                                /* tp_clear */
    0,                                /* tp_richcompare */
    0,                                /* tp_weaklistoffset */
    PyObject_SelfIter,                /* tp_iter */
    0,                                 /* tp_iternext */
    0,                                /* tp_methods */
    0,                                /* tp_members */
    packageDelta_getsetters,        /* tp_getset */
    0,                                /* tp_base */
    0,                                /* tp_dict */
    0,                                /* tp_descr_get */
    0,                                /* tp_descr_set */
    0,                                /* tp_dictoffset */
    0,                                /* tp_init */
    0,                                /* tp_alloc */
    0,                                /* tp_new */
    0,                                /* tp_free */
    0,                                /* tp_is_gc */
};
