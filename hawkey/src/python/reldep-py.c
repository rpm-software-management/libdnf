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
#include "src/reldep_internal.h"
#include "src/sack_internal.h"
#include "src/iutil.h"

// pyhawkey
#include "exception-py.h"
#include "sack-py.h"
#include "reldep-py.h"

#include "pycomp.h"

typedef struct {
    PyObject_HEAD
    HyReldep reldep;
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
    HySack csack = sackFromPyObject(sack);
    if (csack == NULL)
	return NULL;

    _ReldepObject *self = reldep_new_core(&reldep_Type, sack);
    if (self == NULL)
	return NULL;
    self->reldep = reldep_create(sack_pool(csack), r_id);
    return (PyObject*)self;
}

HyReldep
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
reldepToPyObject(HyReldep reldep)
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
    int cmp_type = 0;
    const char *reldep_str = NULL;
    char *name, *evr = NULL;
    if (!PyArg_ParseTuple(args, "O!s", &sack_Type, &sack, &reldep_str))
	return -1;
    HySack csack = sackFromPyObject(sack);
    if (csack == NULL)
	return -1;

    if (parse_reldep_str(reldep_str, &name, &evr, &cmp_type) == -1) {
	PyErr_Format(HyExc_Value, "Wrong reldep format: %s", reldep_str);
	return -1;
    }

    self->reldep = hy_reldep_create(csack, name, cmp_type, evr);
    solv_free(name);
    solv_free(evr);
    if (self->reldep == NULL) {
	PyErr_Format(HyExc_Value, "No such reldep: %s", reldep_str);
	return -1;
    }
    return 0;
}

static void
reldep_dealloc(_ReldepObject *self)
{
    if (self->reldep)
	hy_reldep_free(self->reldep);

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
    HyReldep reldep = self->reldep;
    char *cstr = hy_reldep_str(reldep);
    PyObject *retval = PyString_FromString(cstr);
    solv_free(cstr);
    return retval;
}

static Id
reldep_hash(_ReldepObject *self)
{
    if (self->reldep == NULL) {
	PyErr_SetString(HyExc_Value, "Invalid Reldep has no hash.");
	return -1;
    }
    return reldep_id(self->reldep);
}

PyTypeObject reldep_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hawkey.Reldep",		/*tp_name*/
    sizeof(_ReldepObject),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    (destructor) reldep_dealloc, /*tp_dealloc*/
    0,				/*tp_print*/
    0,				/*tp_getattr*/
    0,				/*tp_setattr*/
    0,				/*tp_compare*/
    (reprfunc)reldep_repr,	/*tp_repr*/
    0,				/*tp_as_number*/
    0,				/*tp_as_sequence*/
    0,				/*tp_as_mapping*/
    (hashfunc)reldep_hash,	/*tp_hash */
    0,				/*tp_call*/
    (reprfunc)reldep_str,	/*tp_str*/
    0,				/*tp_getattro*/
    0,				/*tp_setattro*/
    0,				/*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,	/*tp_flags*/
    "Reldep object",		/* tp_doc */
    0,				/* tp_traverse */
    0,				/* tp_clear */
    0,				/* tp_richcompare */
    0,				/* tp_weaklistoffset */
    0,				/* tp_iter */
    0,                         	/* tp_iternext */
    0,				/* tp_methods */
    0,				/* tp_members */
    0,				/* tp_getset */
    0,				/* tp_base */
    0,				/* tp_dict */
    0,				/* tp_descr_get */
    0,				/* tp_descr_set */
    0,				/* tp_dictoffset */
    (initproc)reldep_init,	/* tp_init */
    0,				/* tp_alloc */
    reldep_new,			/* tp_new */
    0,				/* tp_free */
    0,				/* tp_is_gc */
};
