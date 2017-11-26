/*
 * Copyright (C) 2012-2014 Red Hat, Inc.
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
#include <stdint.h>

// hawkey
#include "hy-repo.h"

// pyhawkey
#include "hawkey-pysys.hpp"
#include "repo-py.hpp"

#include "pycomp.hpp"

typedef struct {
    PyObject_HEAD
    HyRepo repo;
} _RepoObject;

typedef struct {
    int (*getter)(HyRepo);
    void (*setter)(HyRepo, int);
} IntGetSetter;

HyRepo repoFromPyObject(PyObject *o)
{
    if (!repoObject_Check(o)) {
        PyErr_SetString(PyExc_TypeError, "Expected a Repo object.");
        return NULL;
    }
    return ((_RepoObject *)o)->repo;
}

PyObject *repoToPyObject(HyRepo repo)
{
    _RepoObject *self = (_RepoObject *)repo_Type.tp_alloc(&repo_Type, 0);
    if (self)
        self->repo = repo;
    return (PyObject *)self;
}

int
repo_converter(PyObject *o, HyRepo *repo_ptr)
{
    HyRepo repo = repoFromPyObject(o);
    if (repo == NULL)
        return 0;
    *repo_ptr = repo;
    return 1;
}

/* functions on the type */

static PyObject *
repo_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _RepoObject *self = (_RepoObject *)type->tp_alloc(type, 0);
    if (self) {
        self->repo = hy_repo_create("(default)");
        if (self->repo == NULL) {
            Py_DECREF(self);
            return NULL;
        }
    }
    return (PyObject *) self;
}

static int
repo_init(_RepoObject *self, PyObject *args, PyObject *kwds)
{
    const char *name;
    if (!PyArg_ParseTuple(args, "s", &name))
        return -1;
    hy_repo_set_string(self->repo, HY_REPO_NAME, name);
    return 0;
}

static void
repo_dealloc(_RepoObject *self)
{
    hy_repo_free(self->repo);
    Py_TYPE(self)->tp_free(self);
}

/* getsetters */

static PyObject *
get_int(_RepoObject *self, void *closure)
{
    IntGetSetter *functions = (IntGetSetter*)closure;
    return PyLong_FromLong(functions->getter(self->repo));
}

static int
set_int(_RepoObject *self, PyObject *value, void *closure)
{
    IntGetSetter *functions = (IntGetSetter*)closure;
    long num = PyLong_AsLong(value);
    if (PyErr_Occurred())
            return -1;
    if (num > INT_MAX || num < INT_MIN) {
        PyErr_SetString(PyExc_ValueError, "Value in the integer range expected.");
        return -1;
    }
    functions->setter(self->repo, num);
    return 0;
}

static PyObject *
get_str(_RepoObject *self, void *closure)
{
    int str_key = (intptr_t)closure;
    const char *str;
    PyObject *ret;

    str = hy_repo_get_string(self->repo, str_key);
    if (str == NULL) {
        ret = PyString_FromString("");
    } else {
        ret = PyString_FromString(str);
    }
    return ret; // NULL if PyString_FromString failed
}

static int
set_str(_RepoObject *self, PyObject *value, void *closure)
{
    intptr_t str_key = (intptr_t)closure;
    PyObject *tmp_py_str = NULL;
    const char *str_value = pycomp_get_string(value, &tmp_py_str);

    if (str_value == NULL) {
        Py_XDECREF(tmp_py_str);
        return -1;
    }
    hy_repo_set_string(self->repo, str_key, str_value);
    Py_XDECREF(tmp_py_str);

    return 0;
}

static IntGetSetter hy_repo_cost{hy_repo_get_cost, hy_repo_set_cost};

static IntGetSetter hy_repo_priority{hy_repo_get_priority, hy_repo_set_priority};

static PyGetSetDef repo_getsetters[] = {
    {(char*)"cost", (getter)get_int, (setter)set_int, (char*)"repository cost",
     (void *)&hy_repo_cost},
    {(char*)"name", (getter)get_str, (setter)set_str, NULL,
     (void *)HY_REPO_NAME},
    {(char*)"priority", (getter)get_int, (setter)set_int, (char*)"repository priority",
     (void *)&hy_repo_priority},
    {(char*)"repomd_fn", (getter)get_str, (setter)set_str, NULL,
     (void *)HY_REPO_MD_FN},
    {(char*)"primary_fn", (getter)get_str, (setter)set_str, NULL,
     (void *)HY_REPO_PRIMARY_FN},
    {(char*)"filelists_fn", (getter)get_str, (setter)set_str, NULL,
     (void *)HY_REPO_FILELISTS_FN},
    {(char*)"presto_fn", (getter)get_str, (setter)set_str, NULL,
     (void *)HY_REPO_PRESTO_FN},
    {(char*)"updateinfo_fn", (getter)get_str, (setter)set_str, NULL,
     (void *)HY_REPO_UPDATEINFO_FN},
    {NULL}                        /* sentinel */
};

PyTypeObject repo_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hawkey.Repo",                /*tp_name*/
    sizeof(_RepoObject),        /*tp_basicsize*/
    0,                                /*tp_itemsize*/
    (destructor) repo_dealloc,  /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT,                /*tp_flags*/
    "Repo object",                /* tp_doc */
    0,                                /* tp_traverse */
    0,                                /* tp_clear */
    0,                                /* tp_richcompare */
    0,                                /* tp_weaklistoffset */
    PyObject_SelfIter,                /* tp_iter */
    0,                                 /* tp_iternext */
    0,                                /* tp_methods */
    0,                                /* tp_members */
    repo_getsetters,                /* tp_getset */
    0,                                /* tp_base */
    0,                                /* tp_dict */
    0,                                /* tp_descr_get */
    0,                                /* tp_descr_set */
    0,                                /* tp_dictoffset */
    (initproc)repo_init,        /* tp_init */
    0,                                /* tp_alloc */
    repo_new,                        /* tp_new */
    0,                                /* tp_free */
    0,                                /* tp_is_gc */
};
