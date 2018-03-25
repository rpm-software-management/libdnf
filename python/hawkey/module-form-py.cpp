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

// libsolv
#include <solv/util.h>

// hawkey
#include "hy-module-form.hpp"
#include "dnf-sack.h"
#include "hy-types.h"

// pyhawkey
#include "module-form-py.hpp"
#include "pycomp.hpp"

typedef struct {
    PyObject_HEAD
    HyModuleForm module_form;
} _ModuleFormObject;

HyModuleForm
moduleFormFromPyObject(PyObject *o)
{
    if (!PyObject_TypeCheck(o, &module_form_Type)) {
        PyErr_SetString(PyExc_TypeError, "Expected a _hawkey.ModuleForm object.");
        return NULL;
    }
    return ((_ModuleFormObject *)o)->module_form;
}

PyObject *
moduleFormToPyObject(HyModuleForm module_form)
{
    _ModuleFormObject *self = (_ModuleFormObject *)module_form_Type.tp_alloc(&module_form_Type, 0);
    if (self)
        self->module_form = module_form;
    return (PyObject *)self;
}

// getsetters
static bool
set_version(_ModuleFormObject *self, PyObject *value, void *closure)
{
    if (PyInt_Check(value))
        self->module_form->setVersion(PyLong_AsLongLong(value));
    else if (value == Py_None)
        self->module_form->setVersion(libdnf::ModuleForm::VersionNotSet);
    else
        return false;
    return true;
}

static PyObject *
get_version(_ModuleFormObject *self, void *closure)
{
    if (self->module_form->getVersion() == libdnf::ModuleForm::VersionNotSet)
        Py_RETURN_NONE;
    return PyLong_FromLongLong(self->module_form->getVersion());
}

template<const std::string & (libdnf::ModuleForm::*getMethod)() const>
static PyObject *
get_attr(_ModuleFormObject *self, void *closure)
{
    auto str = (self->module_form->*getMethod)();

    if (str.empty())
        Py_RETURN_NONE;
    else
        return PyString_FromString(str.c_str());
}

template<void (libdnf::ModuleForm::*setMethod)(std::string &&)>
static int
set_attr(_ModuleFormObject *self, PyObject *value, void *closure)
{
    PyObject *tmp_py_str = NULL;
    const char *str_value = pycomp_get_string(value, &tmp_py_str);

    if (!str_value) {
        Py_XDECREF(tmp_py_str);
        return -1;
    }
    (self->module_form->*setMethod)(str_value);
    Py_XDECREF(tmp_py_str);

    return 0;
}

static PyGetSetDef module_form_getsetters[] = {
        {(char*)"name", (getter)get_attr<&libdnf::ModuleForm::getName>,
         (setter)set_attr<&libdnf::ModuleForm::setName>, NULL, NULL},
        {(char*)"stream", (getter)get_attr<&libdnf::ModuleForm::getStream>,
         (setter)set_attr<&libdnf::ModuleForm::setStream>, NULL, NULL},
        {(char*)"version", (getter)get_version, (setter)set_version, NULL,
                NULL},
        {(char*)"context", (getter)get_attr<&libdnf::ModuleForm::getContext>,
         (setter)set_attr<&libdnf::ModuleForm::setContext>, NULL, NULL},
        {(char*)"arch", (getter)get_attr<&libdnf::ModuleForm::getArch>,
         (setter)set_attr<&libdnf::ModuleForm::setArch>, NULL, NULL},
        {(char*)"profile", (getter)get_attr<&libdnf::ModuleForm::getProfile>,
         (setter)set_attr<&libdnf::ModuleForm::setProfile>, NULL, NULL},
        {NULL}          /* sentinel */
};

static PyObject *
module_form_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _ModuleFormObject *self = (_ModuleFormObject*)type->tp_alloc(type, 0);
    if (self)
        self->module_form = new libdnf::ModuleForm;
    return (PyObject*)self;
}

static void
module_form_dealloc(_ModuleFormObject *self)
{
    delete self->module_form;
    Py_TYPE(self)->tp_free(self);
}

static int
module_form_init(_ModuleFormObject *self, PyObject *args, PyObject *kwds)
{
    char *name = NULL, *stream = NULL, *context = NULL, *arch = NULL, *profile = NULL;
    PyObject *version_o = NULL;
    HyModuleForm cmodule_form = NULL;

    const char *kwlist[] = {"name", "stream", "version", "context", "arch", "profile",
                            "module_form", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|zzOzzzO&", (char**) kwlist,
                                     &name, &stream, &version_o, &context, &arch, &profile, module_form_converter,
                                     &cmodule_form))
        return -1;
    if (!name && !cmodule_form) {
        PyErr_SetString(PyExc_ValueError, "Name is required parameter.");
        return -1;
    }
    if (cmodule_form) {
        *self->module_form = *cmodule_form;
        return 0;
    }
    if (!set_version(self, version_o, NULL)) {
        PyErr_SetString(PyExc_TypeError, "An integer value or None expected for version.");
        return -1;
    }
    self->module_form->setName(name ? name : "");
    self->module_form->setStream(stream ? stream : "");
    self->module_form->setContext(context ? context : "");
    self->module_form->setArch(arch ? arch : "");
    self->module_form->setProfile(profile ? profile : "");
    return 0;
}

/* object methods */

int
module_form_converter(PyObject *o, HyModuleForm *module_form_ptr)
{
    HyModuleForm module_form = moduleFormFromPyObject(o);
    if (module_form == NULL)
        return 0;
    *module_form_ptr = module_form;
    return 1;
}

static PyObject *
iter(_ModuleFormObject *self)
{
    PyObject *res;
    HyModuleForm mform = self->module_form;
    if (mform->getVersion() == libdnf::ModuleForm::VersionNotSet) {
        Py_INCREF(Py_None);
        res = Py_BuildValue("zzOzzz",
                            mform->getName().empty() ? nullptr : mform->getName().c_str(),
                            mform->getStream().empty() ? nullptr : mform->getStream().c_str(),
                            Py_None,
                            mform->getContext().empty() ? nullptr : mform->getContext().c_str(),
                            mform->getArch().empty() ? nullptr : mform->getArch().c_str(),
                            mform->getProfile().empty() ? nullptr : mform->getProfile().c_str());
    } else
        res = Py_BuildValue("zzLzzz",
                            mform->getName().empty() ? nullptr : mform->getName().c_str(),
                            mform->getStream().empty() ? nullptr : mform->getStream().c_str(),
                            mform->getVersion(),
                            mform->getContext().empty() ? nullptr : mform->getContext().c_str(),
                            mform->getArch().empty() ? nullptr : mform->getArch().c_str(),
                            mform->getProfile().empty() ? nullptr : mform->getProfile().c_str());
    PyObject *iter = PyObject_GetIter(res);
    Py_DECREF(res);
    return iter;
}

PyTypeObject module_form_Type = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "_hawkey.ModuleForm",        /*tp_name*/
        sizeof(_ModuleFormObject),   /*tp_basicsize*/
        0,              /*tp_itemsize*/
        (destructor) module_form_dealloc,  /*tp_dealloc*/
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
        "ModuleForm object",     /* tp_doc */
        0,              /* tp_traverse */
        0,              /* tp_clear */
        0,              /* tp_richcompare */
        0,              /* tp_weaklistoffset */
        (getiterfunc) iter,/* tp_iter */
        0,              /* tp_iternext */
        0,              /* tp_methods */
        0,              /* tp_members */
        module_form_getsetters,       /* tp_getset */
        0,              /* tp_base */
        0,              /* tp_dict */
        0,              /* tp_descr_get */
        0,              /* tp_descr_set */
        0,              /* tp_dictoffset */
        (initproc) module_form_init,   /* tp_init */
        0,              /* tp_alloc */
        module_form_new,          /* tp_new */
        0,              /* tp_free */
        0,              /* tp_is_gc */
};
