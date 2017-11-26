/*
 * Copyright (C) 2014 Red Hat, Inc.
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
#include "dnf-advisorypkg.h"

// pyhawkey
#include "advisorypkg-py.hpp"
#include "iutil-py.hpp"

#include "pycomp.hpp"

typedef struct {
    PyObject_HEAD
    DnfAdvisoryPkg *advisorypkg;
} _AdvisoryPkgObject;


PyObject *
advisorypkgToPyObject(DnfAdvisoryPkg *advisorypkg)
{
    _AdvisoryPkgObject *self = PyObject_New(_AdvisoryPkgObject, &advisorypkg_Type);
    if (!self)
        return NULL;
    self->advisorypkg = advisorypkg;
    return (PyObject *)self;
}

static DnfAdvisoryPkg *
advisorypkgFromPyObject(PyObject *o)
{
    if (!PyObject_TypeCheck(o, &advisorypkg_Type)) {
        PyErr_SetString(PyExc_TypeError, "Expected an AdvisoryPkg object.");
        return NULL;
    }
    return ((_AdvisoryPkgObject*)o)->advisorypkg;
}

static int
advisorypkg_converter(PyObject *o, DnfAdvisoryPkg **ref_ptr)
{
    DnfAdvisoryPkg *ref = advisorypkgFromPyObject(o);
    if (ref == NULL)
        return 0;
    *ref_ptr = ref;
    return 1;
}

/* functions on the type */

static void
advisorypkg_dealloc(_AdvisoryPkgObject *self)
{
    g_object_unref(self->advisorypkg);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
advisorypkg_richcompare(PyObject *self, PyObject *other, int op)
{
    PyObject *result;
    DnfAdvisoryPkg *cself, *cother;

    if (!advisorypkg_converter(self, &cself) ||
        !advisorypkg_converter(other, &cother)) {
        if(PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_TypeError))
            PyErr_Clear();
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    int identical = dnf_advisorypkg_compare(cself, cother);
    switch (op) {
    case Py_EQ:
        result = TEST_COND(identical);
        break;
    case Py_NE:
        result = TEST_COND(!identical);
        break;
    case Py_LE:
    case Py_GE:
    case Py_LT:
    case Py_GT:
        result = Py_NotImplemented;
        break;
    default:
        PyErr_BadArgument();
        return NULL;
    }

    Py_INCREF(result);
    return result;
}

/* getsetters */

static PyObject *
get_attr(_AdvisoryPkgObject *self, void *closure)
{
    intptr_t str_key = (intptr_t)closure;
    if (str_key == 0)
        return PyUnicode_FromString(dnf_advisorypkg_get_name(self->advisorypkg));
    if (str_key == 1)
        return PyUnicode_FromString(dnf_advisorypkg_get_evr(self->advisorypkg));
    if (str_key == 2)
        return PyUnicode_FromString(dnf_advisorypkg_get_arch(self->advisorypkg));
    if (str_key == 3)
        return PyUnicode_FromString(dnf_advisorypkg_get_filename(self->advisorypkg));
    Py_RETURN_NONE;
}

static PyGetSetDef advisorypkg_getsetters[] = {
    {(char*)"name", (getter)get_attr, NULL, NULL, (void *)0},
    {(char*)"evr", (getter)get_attr, NULL, NULL, (void *)1},
    {(char*)"arch", (getter)get_attr, NULL, NULL, (void *)2},
    {(char*)"filename", (getter)get_attr, NULL, NULL, (void *)3},
    {NULL}                      /* sentinel */
};

PyTypeObject advisorypkg_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hawkey.AdvisoryPkg",        /*tp_name*/
    sizeof(_AdvisoryPkgObject),        /*tp_basicsize*/
    0,                                /*tp_itemsize*/
    (destructor) advisorypkg_dealloc,        /*tp_dealloc*/
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
    "AdvisoryPkg object",        /* tp_doc */
    0,                                /* tp_traverse */
    0,                                /* tp_clear */
    advisorypkg_richcompare,        /* tp_richcompare */
    0,                                /* tp_weaklistoffset */
    0,                                /* tp_iter */
    0,                                /* tp_iternext */
    0,                                /* tp_methods */
    0,                                /* tp_members */
    advisorypkg_getsetters,        /* tp_getset */
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
