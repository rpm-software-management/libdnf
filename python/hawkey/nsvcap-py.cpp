/*
 * Copyright (C) 2017 Red Hat, Inc.
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

// pyhawkey
#include "iutil-py.hpp"
#include "nsvcap-py.hpp"
#include "pycomp.hpp"

typedef struct {
    PyObject_HEAD
    libdnf::Nsvcap * nsvcap;
} _NsvcapObject;

libdnf::Nsvcap *
nsvcapFromPyObject(PyObject *o)
{
    if (!PyObject_TypeCheck(o, &nsvcap_Type)) {
        PyErr_SetString(PyExc_TypeError, "Expected a _hawkey.Nsvcap object.");
        return NULL;
    }
    return ((_NsvcapObject *)o)->nsvcap;
}

PyObject *
nsvcapToPyObject(libdnf::Nsvcap * nsvcap)
{
    _NsvcapObject *self = (_NsvcapObject *)nsvcap_Type.tp_alloc(&nsvcap_Type, 0);
    if (self)
        self->nsvcap = nsvcap;
    return (PyObject *)self;
}

// getsetters
template<const std::string & (libdnf::Nsvcap::*getMethod)() const>
static PyObject *
get_attr(_NsvcapObject *self, void *closure)
{
    auto str = (self->nsvcap->*getMethod)();

    if (str.empty())
        Py_RETURN_NONE;
    else
        return PyString_FromString(str.c_str());
}

template<void (libdnf::Nsvcap::*setMethod)(std::string &&)>
static int
set_attr(_NsvcapObject *self, PyObject *value, void *closure)
{
    PycompString str_value(value);
    if (!str_value.getCString())
        return -1;
    (self->nsvcap->*setMethod)(str_value.getCString());
    return 0;
}

static PyGetSetDef nsvcap_getsetters[] = {
        {(char*)"name", (getter)get_attr<&libdnf::Nsvcap::getName>,
         (setter)set_attr<&libdnf::Nsvcap::setName>, NULL, NULL},
        {(char*)"stream", (getter)get_attr<&libdnf::Nsvcap::getStream>,
         (setter)set_attr<&libdnf::Nsvcap::setStream>, NULL, NULL},
        {(char*)"version", (getter)get_attr<&libdnf::Nsvcap::getVersion>,
         (setter)set_attr<&libdnf::Nsvcap::setVersion>, NULL, NULL},
        {(char*)"context", (getter)get_attr<&libdnf::Nsvcap::getContext>,
         (setter)set_attr<&libdnf::Nsvcap::setContext>, NULL, NULL},
        {(char*)"arch", (getter)get_attr<&libdnf::Nsvcap::getArch>,
         (setter)set_attr<&libdnf::Nsvcap::setArch>, NULL, NULL},
        {(char*)"profile", (getter)get_attr<&libdnf::Nsvcap::getProfile>,
         (setter)set_attr<&libdnf::Nsvcap::setProfile>, NULL, NULL},
        {NULL}          /* sentinel */
};

static PyObject *
nsvcap_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _NsvcapObject *self = (_NsvcapObject*)type->tp_alloc(type, 0);
    if (self)
        self->nsvcap = new libdnf::Nsvcap;
    return (PyObject*)self;
}

static void
nsvcap_dealloc(_NsvcapObject *self)
{
    delete self->nsvcap;
    Py_TYPE(self)->tp_free(self);
}

static int
nsvcap_init(_NsvcapObject *self, PyObject *args, PyObject *kwds)
{
    char *name = NULL, *stream = NULL, *version = NULL, *context = NULL, *arch = NULL, *profile = NULL;
    libdnf::Nsvcap * cNsvcap = NULL;

    const char *kwlist[] = {"name", "stream", "version", "context", "arch", "profile",
                            "nsvcap", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|zzzzzzO&", (char**) kwlist,
                                     &name, &stream, &version, &context, &arch, &profile, nsvcapConverter,
                                     &cNsvcap))
        return -1;
    if (!name && !cNsvcap) {
        PyErr_SetString(PyExc_ValueError, "Name is required parameter.");
        return -1;
    }
    if (cNsvcap) {
        *self->nsvcap = *cNsvcap;
        return 0;
    }
    
    self->nsvcap->setName(name);
    if (stream) {
        self->nsvcap->setStream(stream);
    }
    if (version) {
        self->nsvcap->setVersion(version);
    }
    if (context) {
        self->nsvcap->setContext(context);
    }
    if (arch) {
        self->nsvcap->setArch(arch);
    }
    if (profile) {
        self->nsvcap->setProfile(profile);
    }
    return 0;
}

/* object methods */

int
nsvcapConverter(PyObject *o, libdnf::Nsvcap ** nsvcap_ptr)
{
    libdnf::Nsvcap * nsvcap = nsvcapFromPyObject(o);
    if (!nsvcap)
        return 0;
    *nsvcap_ptr = nsvcap;
    return 1;
}

PyTypeObject nsvcap_Type = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "_hawkey.NSVCAP",        /*tp_name*/
        sizeof(_NsvcapObject),   /*tp_basicsize*/
        0,              /*tp_itemsize*/
        (destructor) nsvcap_dealloc,  /*tp_dealloc*/
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
        "NSVCAP object",     /* tp_doc */
        0,              /* tp_traverse */
        0,              /* tp_clear */
        0,              /* tp_richcompare */
        0,              /* tp_weaklistoffset */
        0,              /* tp_iter */
        0,              /* tp_iternext */
        0,              /* tp_methods */
        0,              /* tp_members */
        nsvcap_getsetters,       /* tp_getset */
        0,              /* tp_base */
        0,              /* tp_dict */
        0,              /* tp_descr_get */
        0,              /* tp_descr_set */
        0,              /* tp_dictoffset */
        (initproc) nsvcap_init,   /* tp_init */
        0,              /* tp_alloc */
        nsvcap_new,     /* tp_new */
        0,              /* tp_free */
        0,              /* tp_is_gc */
};
