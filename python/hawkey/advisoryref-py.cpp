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
#include "dnf-advisoryref.h"

// pyhawkey
#include "advisoryref-py.hpp"
#include "iutil-py.hpp"

#include "pycomp.hpp"

typedef struct {
    PyObject_HEAD
    DnfAdvisoryRef *advisoryref;
    PyObject *sack;
} _AdvisoryRefObject;


PyObject *
advisoryrefToPyObject(DnfAdvisoryRef *advisoryref, PyObject *sack)
{
    _AdvisoryRefObject *self = PyObject_New(_AdvisoryRefObject, &advisoryref_Type);
    if (!self)
        return NULL;

    self->advisoryref = advisoryref;
    self->sack = sack;
    Py_INCREF(sack);

    return (PyObject *)self;
}

static DnfAdvisoryRef *
advisoryrefFromPyObject(PyObject *o)
{
    if (!PyObject_TypeCheck(o, &advisoryref_Type)) {
        PyErr_SetString(PyExc_TypeError, "Expected an AdvisoryRef object.");
        return NULL;
    }
    return ((_AdvisoryRefObject*)o)->advisoryref;
}

static int
advisoryref_converter(PyObject *o, DnfAdvisoryRef **ref_ptr)
{
    DnfAdvisoryRef *ref = advisoryrefFromPyObject(o);
    if (ref == NULL)
        return 0;
    *ref_ptr = ref;
    return 1;
}

/* functions on the type */

static void
advisoryref_dealloc(_AdvisoryRefObject *self)
{
    g_object_unref(self->advisoryref);
    Py_XDECREF(self->sack);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
advisoryref_richcompare(PyObject *self, PyObject *other, int op)
{
    PyObject *result;
    DnfAdvisoryRef *cself, *cother;

    if (!advisoryref_converter(self, &cself) ||
        !advisoryref_converter(other, &cother)) {
        if(PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_TypeError))
            PyErr_Clear();
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    int identical = dnf_advisoryref_compare(cself, cother);
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
get_type(_AdvisoryRefObject *self, void *closure)
{
    DnfAdvisoryRefKind (*func)(DnfAdvisoryRef*);
    DnfAdvisoryRefKind ctype;

    func = (DnfAdvisoryRefKind (*)(DnfAdvisoryRef*))closure;
    ctype = func(self->advisoryref);
    return PyLong_FromLong(ctype);
}

static PyObject *
get_str(_AdvisoryRefObject *self, void *closure)
{
    const char *(*func)(DnfAdvisoryRef*);
    const char *cstr;

    func = (const char *(*)(DnfAdvisoryRef*))closure;
    cstr = func(self->advisoryref);
    if (cstr == NULL)
        Py_RETURN_NONE;
    return PyUnicode_FromString(cstr);
}

static PyGetSetDef advisoryref_getsetters[] = {
    {(char*)"type", (getter)get_type, NULL, NULL, (void *)dnf_advisoryref_get_kind},
    {(char*)"id", (getter)get_str, NULL, NULL, (void *)dnf_advisoryref_get_id},
    {(char*)"title", (getter)get_str, NULL, NULL, (void *)dnf_advisoryref_get_title},
    {(char*)"url", (getter)get_str, NULL, NULL, (void *)dnf_advisoryref_get_url},
    {NULL}                      /* sentinel */
};

PyTypeObject advisoryref_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hawkey.AdvisoryRef",        /*tp_name*/
    sizeof(_AdvisoryRefObject),        /*tp_basicsize*/
    0,                                /*tp_itemsize*/
    (destructor) advisoryref_dealloc,        /*tp_dealloc*/
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
    "AdvisoryRef object",        /* tp_doc */
    0,                                /* tp_traverse */
    0,                                /* tp_clear */
    advisoryref_richcompare,        /* tp_richcompare */
    0,                                /* tp_weaklistoffset */
    0,                                /* tp_iter */
    0,                                /* tp_iternext */
    0,                                /* tp_methods */
    0,                                /* tp_members */
    advisoryref_getsetters,        /* tp_getset */
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
