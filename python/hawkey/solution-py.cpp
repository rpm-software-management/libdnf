/*
 * Copyright (C) 2016 Red Hat, Inc.
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
#include <glib.h>
#include <structmember.h>
#include <stddef.h>
#include <solv/util.h>

#include "dnf-types.h"
#include "dnf-solution.h"

#include "solution-py.hpp"

typedef struct {
    PyObject_HEAD
    DnfSolution *solution;
} _SolutionObject;

PyObject *
solutionToPyObject(DnfSolution *solution)
{
    _SolutionObject *self = PyObject_New(_SolutionObject, &solution_Type);
    if (!self)
        return NULL;
    self->solution = solution;
    return (PyObject *)self;
}

PyObject *
solutionlist_to_pylist(const GPtrArray *slist)
{
    auto list = PyList_New(0);
    if (list == NULL)
        return NULL;

    for (unsigned int i = 0; i < slist->len; ++i) {
        auto sol = static_cast<DnfSolution *>(g_object_ref(g_ptr_array_index(slist,  i)));
        auto pysol = solutionToPyObject(sol);
        if (pysol == NULL)
            goto fail;

        int rc = PyList_Append(list, pysol);
        Py_DECREF(pysol);
        if (rc == -1)
            goto fail;
    }

    return list;
 fail:
    Py_DECREF(list);
    return NULL;
}

/* functions on the type */

static void
solution_dealloc(_SolutionObject *self)
{
    g_object_unref(self->solution);
    Py_TYPE(self)->tp_free(self);
}

/* object getters */

static PyObject *
get_action(_SolutionObject *self, void *closure)
{
    return PyLong_FromLong(dnf_solution_get_action(self->solution));
}

static PyObject *
get_old(_SolutionObject *self, void *closure)
{
    const char *s = dnf_solution_get_old(self->solution);
    if (!s)
        Py_RETURN_NONE;
    return PyUnicode_FromString(s);
}

static PyObject *
get_new(_SolutionObject *self, void *closure)
{
    const char *s = dnf_solution_get_new(self->solution);
    if (!s)
        Py_RETURN_NONE;
    return PyUnicode_FromString(s);
}

static PyGetSetDef solution_getsetters[] = {
    {(char*)"action", (getter)get_action, NULL, NULL, NULL},
    {(char*)"old",  (getter)get_old, NULL, NULL, NULL},
    {(char*)"new",  (getter)get_new, NULL, NULL, NULL},
    {NULL}                      /* sentinel */
};

PyTypeObject solution_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hawkey.Solution",               /*tp_name*/
    sizeof(_SolutionObject),          /*tp_basicsize*/
    0,                                /*tp_itemsize*/
    (destructor) solution_dealloc,    /*tp_dealloc*/
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
    0,                                /*tp_getattr*/
    0,                                /*tp_setattr*/
    0,                                /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "Solution object",                /* tp_doc */
    0,                                /* tp_traverse */
    0,                                /* tp_clear */
    0,                                /* tp_richcompare */
    0,                                /* tp_weaklistoffset */
    0,                                /* tp_iter */
    0,                                /* tp_iternext */
    0,                                /* tp_methods */
    0,                                /* tp_members */
    solution_getsetters,              /* tp_getset */
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
