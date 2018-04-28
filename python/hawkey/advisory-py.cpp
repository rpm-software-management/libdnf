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
#include <datetime.h>

// hawkey
#include "dnf-advisory.h"
#include "dnf-advisorypkg.h"
#include "dnf-advisoryref.h"
#include "hy-package.h"

// pyhawkey
#include "advisory-py.hpp"
#include "iutil-py.hpp"

#include "pycomp.hpp"

typedef struct {
    PyObject_HEAD
    DnfAdvisory *advisory;
    PyObject *sack;
} _AdvisoryObject;


PyObject *
advisoryToPyObject(DnfAdvisory *advisory, PyObject *sack)
{
    _AdvisoryObject *self = PyObject_New(_AdvisoryObject, &advisory_Type);
    if (!self)
        return NULL;

    self->advisory = advisory;
    self->sack = sack;
    Py_INCREF(sack);

    return (PyObject *)self;
}

static DnfAdvisory *
advisoryFromPyObject(PyObject *o)
{
    if (!PyObject_TypeCheck(o, &advisory_Type)) {
        PyErr_SetString(PyExc_TypeError, "Expected an Advisory object.");
        return NULL;
    }
    return ((_AdvisoryObject*)o)->advisory;
}

static int
advisory_converter(PyObject *o, DnfAdvisory **advisory_ptr)
{
    DnfAdvisory *advisory = advisoryFromPyObject(o);
    if (advisory == NULL)
        return 0;
    *advisory_ptr = advisory;
    return 1;
}

/* functions on the type */

static void
advisory_dealloc(_AdvisoryObject *self)
{
    dnf_advisory_free(self->advisory);
    Py_XDECREF(self->sack);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
advisory_richcompare(PyObject *self, PyObject *other, int op)
{
    PyObject *result;
    DnfAdvisory *cself, *cother;

    if (!advisory_converter(self, &cself) ||
        !advisory_converter(other, &cother)) {
        if(PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_TypeError))
            PyErr_Clear();
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    int identical = dnf_advisory_compare(cself, cother);
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
get_str(_AdvisoryObject *self, void *closure)
{
    const char *(*func)(DnfAdvisory*);
    const char *cstr;

    func = (const char *(*)(DnfAdvisory*))closure;
    cstr = func(self->advisory);
    if (cstr == NULL)
        Py_RETURN_NONE;
    return PyUnicode_FromString(cstr);
}

static PyObject *
get_type(_AdvisoryObject *self, void *closure)
{
    DnfAdvisoryKind (*func)(DnfAdvisory*);
    DnfAdvisoryKind ctype;

    func = (DnfAdvisoryKind (*)(DnfAdvisory*))closure;
    ctype = func(self->advisory);
    return PyLong_FromLong(ctype);
}

static PyObject *
get_datetime(_AdvisoryObject *self, void *closure)
{
    guint64 (*func)(DnfAdvisory*);
    func = (guint64 (*)(DnfAdvisory*))closure;
    UniquePtrPyObject timestamp(PyLong_FromUnsignedLongLong(func(self->advisory)));
    UniquePtrPyObject args(Py_BuildValue("(O)", timestamp.get()));
    PyDateTime_IMPORT;
    PyObject *datetime = PyDateTime_FromTimestamp(args.get());
    return datetime;
}

static PyObject *
get_advisorypkg_list(_AdvisoryObject *self, void *closure)
{
    std::vector<libdnf::AdvisoryPkg> advisoryPkgs;
    self->advisory->getPackages(advisoryPkgs);
    return advisoryPkgVectorToPylist(advisoryPkgs);
}

static PyObject *
get_advisoryref_list(_AdvisoryObject *self, void *closure)
{
    std::vector<libdnf::AdvisoryRef> advisoryRefs;
    self->advisory->getReferences(advisoryRefs);
    return advisoryRefVectorToPylist(advisoryRefs, self->sack);
}

static PyObject *
matchBugOrCVE(_AdvisoryObject *self, PyObject *args, bool bug)
{
    PyObject *string;
    if (!PyArg_ParseTuple(args, "O", &string))
        return NULL;

    PycompString cmatch(string);
    if (!cmatch.getCString())
        return NULL;

    bool retValue;
    if (bug) {
        retValue = self->advisory->matchBug(cmatch.getCString());
    } else {
        retValue = self->advisory->matchCVE(cmatch.getCString());
    }
    return PyBool_FromLong(retValue);
}

static PyObject *
matchBug(_AdvisoryObject *self, PyObject *args)
{
    return matchBugOrCVE(self, args, true);
}

static PyObject *
matchCVE(_AdvisoryObject *self, PyObject *args)
{
    return matchBugOrCVE(self, args, false);
}

static PyGetSetDef advisory_getsetters[] = {
    {(char*)"title", (getter)get_str, NULL, NULL, (void *)dnf_advisory_get_title},
    {(char*)"id", (getter)get_str, NULL, NULL, (void *)dnf_advisory_get_id},
    {(char*)"type", (getter)get_type, NULL, NULL, (void *)dnf_advisory_get_kind},
    {(char*)"description", (getter)get_str, NULL, NULL, (void *)dnf_advisory_get_description},
    {(char*)"rights", (getter)get_str, NULL, NULL, (void *)dnf_advisory_get_rights},
    {(char*)"severity", (getter)get_str, NULL, NULL, (void *)dnf_advisory_get_severity},
    {(char*)"updated", (getter)get_datetime, NULL, NULL, (void *)dnf_advisory_get_updated},
    {(char*)"packages", (getter)get_advisorypkg_list, NULL, NULL, NULL},
    {(char*)"references", (getter)get_advisoryref_list, NULL, NULL, NULL},
    {NULL}                      /* sentinel */
};

static struct PyMethodDef advisory_methods[] = {
    {"match_bug", (PyCFunction)matchBug, METH_VARARGS, NULL},
    {"match_cve", (PyCFunction)matchCVE, METH_VARARGS, NULL},
    {NULL}                      /* sentinel */
};

PyTypeObject advisory_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hawkey.Advisory",                /*tp_name*/
    sizeof(_AdvisoryObject),        /*tp_basicsize*/
    0,                                /*tp_itemsize*/
    (destructor) advisory_dealloc,        /*tp_dealloc*/
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
    "Advisory object",                /* tp_doc */
    0,                                /* tp_traverse */
    0,                                /* tp_clear */
    advisory_richcompare,        /* tp_richcompare */
    0,                                /* tp_weaklistoffset */
    0,                                /* tp_iter */
    0,                                /* tp_iternext */
    advisory_methods,                 /* tp_methods */
    0,                                /* tp_members */
    advisory_getsetters,        /* tp_getset */
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
