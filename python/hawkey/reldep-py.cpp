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

#include <assert.h>
#include <Python.h>

// libsolv
#include <solv/util.h>

// hawkey
#include "dnf-reldep.h"
#include "hy-iutil.h"
#include "libdnf/repo/solvable/Dependency.hpp"

// pyhawkey
#include "exception-py.hpp"
#include "sack-py.hpp"
#include "reldep-py.hpp"
#include "iutil-py.hpp"

#include "pycomp.hpp"

typedef struct {
    PyObject_HEAD
    DnfReldep *reldep;
    PyObject *sack;
} _ReldepObject;

static _ReldepObject *
reldep_new_core(PyTypeObject *type, PyObject *sack)
{
    _ReldepObject *self = (_ReldepObject*)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;
    self->reldep = NULL;
    self->sack = sack;
    Py_INCREF(self->sack);
    return self;
}

PyObject *
new_reldep(PyObject *sack, Id r_id)
{
    DnfSack *csack = sackFromPyObject(sack);
    if (csack == NULL)
        return NULL;

    _ReldepObject *self = reldep_new_core(&reldep_Type, sack);
    if (self == NULL)
        return NULL;
    self->reldep = new libdnf::Dependency(csack, r_id);
    return (PyObject*)self;
}

DnfReldep *
reldepFromPyObject(PyObject *o)
{
    if (!PyType_IsSubtype(o->ob_type, &reldep_Type)) {
        PyErr_SetString(PyExc_TypeError, "Expected a Reldep object.");
        return NULL;
    }
    return ((_ReldepObject*)o)->reldep;
}

static Id reldep_hash(_ReldepObject *self);

static PyObject *
reldep_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *sack = PyTuple_GetItem(args, 0);
    if (sack == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "Expected a Sack object as the first argument.");
        return NULL;
    }
    if (!sackObject_Check(sack)) {
        PyErr_SetString(PyExc_TypeError,
                        "Expected a Sack object as the first argument.");
        return NULL;
    }
    return (PyObject *)reldep_new_core(type, sack);
}

PyObject *
reldepToPyObject(DnfReldep *reldep)
{
    _ReldepObject *self = (_ReldepObject *)reldep_Type.tp_alloc(&reldep_Type, 0);
    if (self)
        self->reldep = reldep;
    return (PyObject *)self;
}

static int
reldep_init(_ReldepObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *sack;
    PyObject *reldep_str_py = NULL;
    if (!PyArg_ParseTuple(args, "O!O", &sack_Type, &sack, &reldep_str_py))
        return -1;
    DnfSack *csack = sackFromPyObject(sack);
    if (csack == NULL)
        return -1;
    PycompString reldep_str(reldep_str_py);
    if (!reldep_str.getCString())
        return -1;

    try {
        self->reldep = new libdnf::Dependency(csack, reldep_str.getCString());
    }
    catch (...) {
        PyErr_Format(HyExc_Value, "Wrong reldep format: %s", reldep_str.getCString());
        return -1;
    }
    return 0;
}

static void
reldep_dealloc(_ReldepObject *self)
{
    if (self->reldep)
        dnf_reldep_free(self->reldep);

    Py_XDECREF(self->sack);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
reldep_repr(_ReldepObject *self)
{
    long hash = reldep_hash(self);
    if (PyErr_Occurred()) {
        assert(hash == -1);
        PyErr_Clear();
        return PyString_FromString("<_hawkey.Reldep object, INVALID value>");
    }
    return PyString_FromFormat("<_hawkey.Reldep object, id: %lu>", hash);
}

static PyObject *
reldep_str(_ReldepObject *self)
{
    DnfReldep *reldep = self->reldep;
    const char *cstr = reldep->toString();
    PyObject *retval = PyString_FromString(cstr);
    return retval;
}

static Id
reldep_hash(_ReldepObject *self)
{
    if (self->reldep == NULL) {
        PyErr_SetString(HyExc_Value, "Invalid Reldep has no hash.");
        return -1;
    }
    return self->reldep->getId();
}

static int
reldep_converter(PyObject *o, DnfReldep **reldep_ptr)
{
    DnfReldep *reldep = reldepFromPyObject(o);
    if (reldep == NULL)
        return 0;
    *reldep_ptr = reldep;
    return 1;
}

static PyObject *
reldep_richcompare(PyObject *self, PyObject *other, int op)
{
    PyObject *result = NULL;
    DnfReldep *cself, *cother;

    if (!reldep_converter(self, &cself) ||
        !reldep_converter(other, &cother)) {
        if(PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_TypeError))
            PyErr_Clear();
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    Id s_id = cself->getId();
    Id o_id = cother->getId();
    switch (op) {
    case Py_EQ:
        result = TEST_COND(s_id == o_id);
        break;
    case Py_NE:
        result = TEST_COND(s_id != o_id);
        break;
    case Py_LT:
    case Py_LE:
    case Py_GT:
    case Py_GE:
        result = Py_NotImplemented;
        break;
    default:
        PyErr_BadArgument();
        return NULL;
    }

    Py_INCREF(result);
    return result;
}

PyTypeObject reldep_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hawkey.Reldep",                /*tp_name*/
    sizeof(_ReldepObject),        /*tp_basicsize*/
    0,                                /*tp_itemsize*/
    (destructor) reldep_dealloc, /*tp_dealloc*/
    0,                                /*tp_print*/
    0,                                /*tp_getattr*/
    0,                                /*tp_setattr*/
    0,                                /*tp_compare*/
    (reprfunc)reldep_repr,        /*tp_repr*/
    0,                                /*tp_as_number*/
    0,                                /*tp_as_sequence*/
    0,                                /*tp_as_mapping*/
    (hashfunc)reldep_hash,        /*tp_hash */
    0,                                /*tp_call*/
    (reprfunc)reldep_str,        /*tp_str*/
    0,                                /*tp_getattro*/
    0,                                /*tp_setattro*/
    0,                                /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "Reldep object",                /* tp_doc */
    0,                                /* tp_traverse */
    0,                                /* tp_clear */
    (richcmpfunc)reldep_richcompare, /* tp_richcompare */
    0,                                /* tp_weaklistoffset */
    0,                                /* tp_iter */
    0,                                 /* tp_iternext */
    0,                                /* tp_methods */
    0,                                /* tp_members */
    0,                                /* tp_getset */
    0,                                /* tp_base */
    0,                                /* tp_dict */
    0,                                /* tp_descr_get */
    0,                                /* tp_descr_set */
    0,                                /* tp_dictoffset */
    (initproc)reldep_init,        /* tp_init */
    0,                                /* tp_alloc */
    reldep_new,                        /* tp_new */
    0,                                /* tp_free */
    0,                                /* tp_is_gc */
};
