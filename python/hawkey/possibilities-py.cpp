/*
 * Copyright (C) 2013 Red Hat, Inc.
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
#include "hy-subject.h"

// pyhawkey
#include "nevra-py.hpp"
#include "module-form-py.hpp"
#include "possibilities-py.hpp"
#include "pycomp.hpp"
#include "reldep-py.hpp"

typedef struct {
    PyObject_HEAD
    HyPossibilities possibilities;
    PyObject *sack;
} _PossibilitiesObject;

PyObject *
possibilitiesToPyObject(HyPossibilities possibilities, PyObject* sack)
{
    _PossibilitiesObject *p;

    p = PyObject_New(_PossibilitiesObject, &possibilities_Type);
    if (!p)
        return NULL;

    if (!PyObject_Init((PyObject *)p, &possibilities_Type)) {
        Py_DECREF(p);
        return NULL;
    }
    p->possibilities = possibilities;
    p->sack = sack;
    Py_XINCREF(p->sack);
    return (PyObject *)p;
}

static void
possibilities_dealloc(_PossibilitiesObject *self)
{
    hy_possibilities_free(self->possibilities);
    Py_XDECREF(self->sack);
    Py_TYPE(self)->tp_free(self);
}

static PyObject* possibilities_iter(PyObject *self)
{
    Py_INCREF(self);
    return self;
}

static PyObject* possibilities_next(_PossibilitiesObject *self)
{
    HyPossibilities iter = self->possibilities;
    if (iter->type == TYPE_NEVRA) {
        HyNevra nevra;
        if (hy_possibilities_next_nevra(iter, &nevra) == 0)
            return nevraToPyObject(nevra);
    } else if (iter->type == TYPE_MODULE_FORM) {
        HyModuleForm module_form;
        if (hy_possibilities_next_module_form(iter, &module_form) == 0)
            return moduleFormToPyObject(module_form);
    } else {
        DnfReldep *reldep;
        if (hy_possibilities_next_reldep(iter, &reldep) == 0)
            return reldepToPyObject(reldep);
    }
    PyErr_SetNone(PyExc_StopIteration);
    return NULL;
}

PyTypeObject possibilities_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hawkey.Possibilities",            /*tp_name*/
    sizeof(_PossibilitiesObject),       /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor) possibilities_dealloc,  /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_ITER, /* tp_flags */
    "Possibilities iterator object",           /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    possibilities_iter,        /* tp_iter: __iter__() method */
    (iternextfunc) possibilities_next,  /* tp_iternext: next() method */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
    0,                         /* tp_free */
    0,                         /* tp_is_gc */
};
