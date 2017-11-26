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
#include "hy-module-form.h"
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
static long long
set_version(_ModuleFormObject *self, PyObject *value, void *closure)
{
    if (PyInt_Check(value))
        hy_module_form_set_version(self->module_form, PyLong_AsLong(value));
    else if (value == Py_None)
        hy_module_form_set_version(self->module_form, -1);
    else
        return -1;
    return 0;
}

static PyObject *
get_version(_ModuleFormObject *self, void *closure)
{
    if (hy_module_form_get_version(self->module_form) == -1L)
        Py_RETURN_NONE;
#if PY_MAJOR_VERSION >= 3
    return PyLong_FromLong(hy_module_form_get_version(self->module_form));
#else
    return PyInt_FromLong(hy_module_form_get_version(self->module_form));
#endif
}

static PyObject *
get_attr(_ModuleFormObject *self, void *closure)
{
    intptr_t str_key = (intptr_t)closure;
    const char *str;

    str = hy_module_form_get_string(self->module_form, str_key);
    if (str == NULL)
        Py_RETURN_NONE;
    else
        return PyString_FromString(str);
}

static int
set_attr(_ModuleFormObject *self, PyObject *value, void *closure)
{
    intptr_t str_key = (intptr_t)closure;
    PyObject *tmp_py_str = NULL;
    const char *str_value = pycomp_get_string(value, &tmp_py_str);

    if (str_value == NULL) {
        Py_XDECREF(tmp_py_str);
        return -1;
    }
    hy_module_form_set_string(self->module_form, str_key, str_value);
    Py_XDECREF(tmp_py_str);

    return 0;
}

static PyGetSetDef module_form_getsetters[] = {
        {(char*)"name", (getter)get_attr, (setter)set_attr, NULL,
                (void *)HY_MODULE_FORM_NAME},
        {(char*)"stream", (getter)get_attr, (setter)set_attr, NULL,
                (void *)HY_MODULE_FORM_STREAM},
        {(char*)"version", (getter)get_version, (setter)set_version, NULL,
                NULL},
        {(char*)"context", (getter)get_attr, (setter)set_attr, NULL,
                (void *)HY_MODULE_FORM_CONTEXT},
        {(char*)"arch", (getter)get_attr, (setter)set_attr, NULL,
                (void *)HY_MODULE_FORM_ARCH},
        {(char*)"profile", (getter)get_attr, (setter)set_attr, NULL,
                (void *)HY_MODULE_FORM_PROFILE},
        {NULL}          /* sentinel */
};

static PyObject *
module_form_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _ModuleFormObject *self = (_ModuleFormObject*)type->tp_alloc(type, 0);
    if (self) {
        self->module_form = hy_module_form_create();
    }
    return (PyObject*)self;
}

static void
module_form_dealloc(_ModuleFormObject *self)
{
    hy_module_form_free(self->module_form);
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
    if (name == NULL && cmodule_form == NULL) {
        PyErr_SetString(PyExc_ValueError, "Name is required parameter.");
        return -1;
    }
    if (cmodule_form != NULL) {
        self->module_form = hy_module_form_clone(cmodule_form);
        return 0;
    }
    if (set_version(self, version_o, NULL) == -1) {
        PyErr_SetString(PyExc_TypeError, "An integer value or None expected for version.");
        return -1;
    }
    hy_module_form_set_string(self->module_form, HY_MODULE_FORM_NAME, name);
    hy_module_form_set_string(self->module_form, HY_MODULE_FORM_STREAM, stream);
    hy_module_form_set_string(self->module_form, HY_MODULE_FORM_CONTEXT, context);
    hy_module_form_set_string(self->module_form, HY_MODULE_FORM_ARCH, arch);
    hy_module_form_set_string(self->module_form, HY_MODULE_FORM_PROFILE, profile);
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
    HyModuleForm module_form = self->module_form;
    if (hy_module_form_get_version(self->module_form) == -1) {
        Py_INCREF(Py_None);
        res = Py_BuildValue("zzOzzz",
                            hy_module_form_get_string(module_form, HY_MODULE_FORM_NAME),
                            hy_module_form_get_string(module_form, HY_MODULE_FORM_STREAM),
                            Py_None,
                            hy_module_form_get_string(module_form, HY_MODULE_FORM_CONTEXT),
                            hy_module_form_get_string(module_form, HY_MODULE_FORM_ARCH),
                            hy_module_form_get_string(module_form, HY_MODULE_FORM_PROFILE));
    } else
        res = Py_BuildValue("zzLzzz",
                            hy_module_form_get_string(module_form, HY_MODULE_FORM_NAME),
                            hy_module_form_get_string(module_form, HY_MODULE_FORM_STREAM),
                            hy_module_form_get_string(module_form, HY_MODULE_FORM_VERSION),
                            hy_module_form_get_string(module_form, HY_MODULE_FORM_CONTEXT),
                            hy_module_form_get_string(module_form, HY_MODULE_FORM_ARCH),
                            hy_module_form_get_string(module_form, HY_MODULE_FORM_PROFILE));
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
