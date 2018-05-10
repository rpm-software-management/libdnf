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
#include "hy-query.h"
#include "dnf-sack.h"

// pyhawkey
#include "iutil-py.hpp"
#include "nevra-py.hpp"
#include "pycomp.hpp"
#include "query-py.hpp"
#include "sack-py.hpp"

typedef struct {
    PyObject_HEAD
    libdnf::Nevra *nevra;
} _NevraObject;

libdnf::Nevra *
nevraFromPyObject(PyObject *o)
{
    if (!PyObject_TypeCheck(o, &nevra_Type)) {
        PyErr_SetString(PyExc_TypeError, "Expected a _hawkey.NEVRA object.");
        return NULL;
    }
    return ((_NevraObject *)o)->nevra;
}

PyObject *
nevraToPyObject(libdnf::Nevra *nevra)
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
        self->nevra->setEpoch(libdnf::Nevra::EPOCH_NOT_SET);
    else if (PyInt_Check(value))
        self->nevra->setEpoch(PyLong_AsLong(value));
    else if (value == Py_None)
        self->nevra->setEpoch(libdnf::Nevra::EPOCH_NOT_SET);
    else
        return -1;
    return 0;
}

static PyObject *
get_epoch(_NevraObject *self, void *closure)
{
    if (self->nevra->getEpoch() == libdnf::Nevra::EPOCH_NOT_SET)
        Py_RETURN_NONE;
#if PY_MAJOR_VERSION >= 3
    return PyLong_FromLong(self->nevra->getEpoch());
#else
    return PyInt_FromLong(self->nevra->getEpoch());
#endif
}

template<const std::string & (libdnf::Nevra::*getMethod)() const>
static PyObject *
get_attr(_NevraObject *self, void *closure)
{
    auto str = (self->nevra->*getMethod)();
    if (str.empty())
        Py_RETURN_NONE;
    else
        return PyString_FromString(str.c_str());
}

template<void (libdnf::Nevra::*setMethod)(std::string &&)>
static int
set_attr(_NevraObject *self, PyObject *value, void *closure)
{
    PycompString str_value(value);
    if (!str_value.getCString())
        return -1;
    (self->nevra->*setMethod)(str_value.getCString());
    return 0;
}

static PyGetSetDef nevra_getsetters[] = {
    {(char*)"name", (getter)get_attr<&libdnf::Nevra::getName>,
        (setter)set_attr<&libdnf::Nevra::setName>, NULL, NULL},
    {(char*)"epoch", (getter)get_epoch, (setter)set_epoch,
        NULL, NULL},
    {(char*)"version", (getter)get_attr<&libdnf::Nevra::getVersion>,
        (setter)set_attr<&libdnf::Nevra::setVersion>, NULL, NULL},
    {(char*)"release", (getter)get_attr<&libdnf::Nevra::getRelease>,
        (setter)set_attr<&libdnf::Nevra::setRelease>, NULL, NULL},
    {(char*)"arch", (getter)get_attr<&libdnf::Nevra::getArch>,
        (setter)set_attr<&libdnf::Nevra::setArch>, NULL, NULL},
    {NULL}          /* sentinel */
};

static PyObject *
nevra_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _NevraObject *self = (_NevraObject*)type->tp_alloc(type, 0);
    if (self)
        self->nevra = new libdnf::Nevra;
    return (PyObject*)self;
}

static void
nevra_dealloc(_NevraObject *self)
{
    delete self->nevra;
    Py_TYPE(self)->tp_free(self);
}

static int
nevra_init(_NevraObject *self, PyObject *args, PyObject *kwds)
{
    char *name = NULL, *version = NULL, *release = NULL, *arch = NULL;
    PyObject *epoch_o = NULL;
    libdnf::Nevra * cnevra = NULL;

    const char *kwlist[] = {"name", "epoch", "version", "release", "arch",
        "nevra", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|zOzzzO&", (char**) kwlist,
        &name, &epoch_o, &version, &release, &arch, nevra_converter, &cnevra))
        return -1;
    if (!name && !cnevra) {
        PyErr_SetString(PyExc_ValueError,
            "Name is required parameter.");
        return -1;
    }
    if (cnevra) {
        *self->nevra = *cnevra;
        return 0;
    }
    if (set_epoch(self, epoch_o, NULL) == -1) {
        PyErr_SetString(PyExc_TypeError,
            "An integer value or None expected for epoch.");
        return -1;
    }
    if (name)
        self->nevra->setName(name);
    if (version)
        self->nevra->setVersion(version);
    if (release)
        self->nevra->setRelease(release);
    if (arch)
        self->nevra->setArch(arch);
    return 0;
}

/* object methods */

static PyObject *
evr(_NevraObject *self, PyObject *unused)
{
    return PyString_FromString(self->nevra->getEvr().c_str());;
}

int
nevra_converter(PyObject *o, libdnf::Nevra **nevra_ptr)
{
    auto nevra = nevraFromPyObject(o);
    if (nevra == NULL)
        return 0;
    *nevra_ptr = nevra;
    return 1;
}

static PyObject *
evr_cmp(_NevraObject *self, PyObject *args)
{
    DnfSack *sack;
    libdnf::Nevra *nevra;
    if (!PyArg_ParseTuple(args, "O&O&", nevra_converter, &nevra, sack_converter, &sack)) {
        return NULL;
    }
    if (sack == NULL || nevra == NULL)
        return NULL;
    int cmp = self->nevra->compareEvr(*nevra, sack);
    return PyLong_FromLong(cmp);
}

static PyObject *
has_just_name(_NevraObject *self, PyObject *unused)
{
    return PyBool_FromLong(self->nevra->hasJustName());
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
    HyQuery query = hy_query_from_nevra(self->nevra, csack, c_icase);
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
    libdnf::Nevra *other_nevra, *self_nevra;
    other_nevra = nevraFromPyObject(other);
    self_nevra = nevraFromPyObject(self);

    if (!other_nevra) {
        if(PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_TypeError))
            PyErr_Clear();
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    long result = self_nevra->compare(*other_nevra);

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
    0,              /* tp_iter */
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
