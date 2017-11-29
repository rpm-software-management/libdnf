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

// libsolv
#include <solv/util.h>

// hawkey
#include "hy-nevra.h"
#include "hy-nevra.h"
#include "dnf-sack.h"
#include "hy-types.h"

// pyhawkey
#include "iutil-py.hpp"
#include "nevra-py.hpp"
#include "pycomp.hpp"
#include "query-py.hpp"
#include "sack-py.hpp"

typedef struct {
    PyObject_HEAD
    HyNevra nevra;
} _NevraObject;

HyNevra
nevraFromPyObject(PyObject *o)
{
    if (!PyObject_TypeCheck(o, &nevra_Type)) {
        PyErr_SetString(PyExc_TypeError, "Expected a _hawkey.NEVRA object.");
        return NULL;
    }
    return ((_NevraObject *)o)->nevra;
}

PyObject *
nevraToPyObject(HyNevra nevra)
{
    _NevraObject *self = (_NevraObject *)nevra_Type.tp_alloc(&nevra_Type, 0);
    if (self)
        self->nevra = nevra;
    return (PyObject *)self;
}

// getsetters
static int
set_epoch(_NevraObject *self, PyObject *value, void *closure)
{
    if (value == NULL)
        hy_nevra_set_epoch(self->nevra, -1);
    else if (PyInt_Check(value))
        hy_nevra_set_epoch(self->nevra, PyLong_AsLong(value));
    else if (value == Py_None)
        hy_nevra_set_epoch(self->nevra, -1);
    else
        return -1;
    return 0;
}

static PyObject *
get_epoch(_NevraObject *self, void *closure)
{
    if (hy_nevra_get_epoch(self->nevra) == -1L)
    Py_RETURN_NONE;
#if PY_MAJOR_VERSION >= 3
    return PyLong_FromLong(hy_nevra_get_epoch(self->nevra));
#else
    return PyInt_FromLong(hy_nevra_get_epoch(self->nevra));
#endif
}

static PyObject *
get_attr(_NevraObject *self, void *closure)
{
    intptr_t str_key = (intptr_t)closure;
    const char *str;

    str = hy_nevra_get_string(self->nevra, str_key);
    if (str == NULL)
        Py_RETURN_NONE;
    else
        return PyString_FromString(str);
}

static int
set_attr(_NevraObject *self, PyObject *value, void *closure)
{
    intptr_t str_key = (intptr_t)closure;
    PyObject *tmp_py_str = NULL;
    const char *str_value = pycomp_get_string(value, &tmp_py_str);

    if (str_value == NULL) {
        Py_XDECREF(tmp_py_str);
        return -1;
    }
    hy_nevra_set_string(self->nevra, str_key, str_value);
    Py_XDECREF(tmp_py_str);

    return 0;
}

static PyGetSetDef nevra_getsetters[] = {
    {(char*)"name", (getter)get_attr, (setter)set_attr, NULL,
     (void *)HY_NEVRA_NAME},
    {(char*)"epoch", (getter)get_epoch, (setter)set_epoch, NULL,
     NULL},
    {(char*)"version", (getter)get_attr, (setter)set_attr, NULL,
     (void *)HY_NEVRA_VERSION},
    {(char*)"release", (getter)get_attr, (setter)set_attr, NULL,
     (void *)HY_NEVRA_RELEASE},
    {(char*)"arch", (getter)get_attr, (setter)set_attr, NULL,
     (void *)HY_NEVRA_ARCH},
    {NULL}          /* sentinel */
};

static PyObject *
nevra_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _NevraObject *self = (_NevraObject*)type->tp_alloc(type, 0);
    if (self) {
        self->nevra = hy_nevra_create();
    }
    return (PyObject*)self;
}

static void
nevra_dealloc(_NevraObject *self)
{
    hy_nevra_free(self->nevra);
    Py_TYPE(self)->tp_free(self);
}

static int
nevra_init(_NevraObject *self, PyObject *args, PyObject *kwds)
{
    char *name = NULL, *version = NULL, *release = NULL, *arch = NULL;
    PyObject *epoch_o = NULL;
    HyNevra cnevra = NULL;

    const char *kwlist[] = {"name", "epoch", "version", "release", "arch",
        "nevra", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|zOzzzO&", (char**) kwlist,
        &name, &epoch_o, &version, &release, &arch, nevra_converter, &cnevra))
        return -1;
    if (name == NULL && cnevra == NULL) {
        PyErr_SetString(PyExc_ValueError,
            "Name is required parameter.");
        return -1;
    }
    if (cnevra != NULL) {
        self->nevra = hy_nevra_clone(cnevra);
        return 0;
    }
    if (set_epoch(self, epoch_o, NULL) == -1) {
        PyErr_SetString(PyExc_TypeError,
            "An integer value or None expected for epoch.");
        return -1;
    }
    hy_nevra_set_string(self->nevra, HY_NEVRA_NAME, name);
    hy_nevra_set_string(self->nevra, HY_NEVRA_VERSION, version);
    hy_nevra_set_string(self->nevra, HY_NEVRA_RELEASE, release);
    hy_nevra_set_string(self->nevra, HY_NEVRA_ARCH, arch);
    return 0;
}

/* object methods */

static PyObject *
evr(_NevraObject *self, PyObject *unused)
{
    char *str;
    PyObject *o;
    str = hy_nevra_get_evr(self->nevra);
    o = PyString_FromString(str);
    g_free(str);
    return o;
}

int
nevra_converter(PyObject *o, HyNevra *nevra_ptr)
{
    HyNevra nevra = nevraFromPyObject(o);
    if (nevra == NULL)
        return 0;
    *nevra_ptr = nevra;
    return 1;
}

static PyObject *
evr_cmp(_NevraObject *self, PyObject *args)
{
    DnfSack *sack;
    HyNevra nevra;
    if (!PyArg_ParseTuple(args, "O&O&", nevra_converter, &nevra, sack_converter, &sack)) {
        return NULL;
    }
    if (sack == NULL || nevra == NULL)
        return NULL;
    int cmp = hy_nevra_evr_cmp(self->nevra, nevra, sack);
    return PyLong_FromLong(cmp);
}

static PyObject *
has_just_name(_NevraObject *self, PyObject *unused)
{
    return PyBool_FromLong(hy_nevra_has_just_name(self->nevra));
}

static PyObject *
to_query(_NevraObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *sack;
    DnfSack *csack;
    const char *kwlist[] = {"sack", "icase", NULL};
    PyObject *icase = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", (char**) kwlist, &sack_Type, &sack,
        &PyBool_Type, &icase)) {
        return NULL;
    }
    gboolean c_icase = icase!=NULL && PyObject_IsTrue(icase);
    csack = sackFromPyObject(sack);
    HyQuery query = hy_nevra_to_query(self->nevra, csack, c_icase);
    PyObject *q = queryToPyObject(query, sack, &query_Type);
    return q;
}

static struct PyMethodDef nevra_methods[] = {
    {"evr_cmp",     (PyCFunction) evr_cmp, METH_VARARGS, NULL},
    {"evr", (PyCFunction) evr, METH_NOARGS,   NULL},
    {"has_just_name",     (PyCFunction) has_just_name, METH_NOARGS, NULL},
    {"to_query",     (PyCFunction) to_query, METH_VARARGS | METH_KEYWORDS, NULL},

    {NULL}                      /* sentinel */
};

static PyObject *
nevra_richcompare(PyObject *self, PyObject *other, int op)
{
    PyObject *v;
    HyNevra other_nevra, self_nevra;
    other_nevra = nevraFromPyObject(other);
    self_nevra = nevraFromPyObject(self);

    if (!other_nevra) {
        if(PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_TypeError))
            PyErr_Clear();
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    long result = hy_nevra_cmp(self_nevra, other_nevra);

    switch (op) {
    case Py_EQ:
        v = TEST_COND(result == 0);
        break;
    case Py_NE:
        v = TEST_COND(result != 0);
        break;
    case Py_LE:
        v = TEST_COND(result <= 0);
        break;
    case Py_GE:
        v = TEST_COND(result >= 0);
        break;
    case Py_LT:
        v = TEST_COND(result < 0);
        break;
    case Py_GT:
        v = TEST_COND(result > 0);
        break;
    default:
        PyErr_BadArgument();
        return NULL;
    }
    Py_INCREF(v);
    return v;
}

static PyObject *
iter(_NevraObject *self)
{
    PyObject *res;
    HyNevra nevra = self->nevra;
    if (hy_nevra_get_epoch(nevra) == -1) {
        Py_INCREF(Py_None);
        res = Py_BuildValue("zOzzz",
                            hy_nevra_get_string(nevra, HY_NEVRA_NAME),
                            Py_None,
                            hy_nevra_get_string(nevra, HY_NEVRA_VERSION),
                            hy_nevra_get_string(nevra, HY_NEVRA_RELEASE),
                            hy_nevra_get_string(nevra, HY_NEVRA_ARCH));
    } else
        res = Py_BuildValue("zizzz",
                            hy_nevra_get_string(nevra, HY_NEVRA_NAME),
                            hy_nevra_get_epoch(nevra),
                            hy_nevra_get_string(nevra, HY_NEVRA_VERSION),
                            hy_nevra_get_string(nevra, HY_NEVRA_RELEASE),
                            hy_nevra_get_string(nevra, HY_NEVRA_ARCH));
    PyObject *iter = PyObject_GetIter(res);
    Py_DECREF(res);
    return iter;
}

PyTypeObject nevra_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hawkey.NEVRA",        /*tp_name*/
    sizeof(_NevraObject),   /*tp_basicsize*/
    0,              /*tp_itemsize*/
    (destructor) nevra_dealloc,  /*tp_dealloc*/
    0,              /*tp_print*/
    0,              /*tp_getattr*/
    0,              /*tp_setattr*/
    0,              /*tp_compare*/
    0,              /*tp_repr*/
    0,              /*tp_as_number*/
    0,              /*tp_as_sequence*/
    0,              /*tp_as_mapping*/
    0,              /*tp_hash */
    0,              /*tp_call*/
    0,              /*tp_str*/
    0,              /*tp_getattro*/
    0,              /*tp_setattro*/
    0,              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,     /*tp_flags*/
    "NEVRA object",     /* tp_doc */
    0,              /* tp_traverse */
    0,              /* tp_clear */
    nevra_richcompare,              /* tp_richcompare */
    0,              /* tp_weaklistoffset */
    (getiterfunc) iter,/* tp_iter */
    0,                          /* tp_iternext */
    nevra_methods,      /* tp_methods */
    0,              /* tp_members */
    nevra_getsetters,       /* tp_getset */
    0,              /* tp_base */
    0,              /* tp_dict */
    0,              /* tp_descr_get */
    0,              /* tp_descr_set */
    0,              /* tp_dictoffset */
    (initproc)nevra_init,   /* tp_init */
    0,              /* tp_alloc */
    nevra_new,          /* tp_new */
    0,              /* tp_free */
    0,              /* tp_is_gc */
};
