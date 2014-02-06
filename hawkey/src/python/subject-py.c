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
#include "src/iutil.h"
#include "src/nevra.h"
#include "src/nevra_internal.h"
#include "src/reldep.h"
#include "src/sack.h"
#include "src/sack_internal.h"
#include "src/subject.h"
#include "src/types.h"

// pyhawkey
#include "iutil-py.h"
#include "nevra-py.h"
#include "possibilities-py.h"
#include "pycomp.h"
#include "reldep-py.h"
#include "sack-py.h"
#include "subject-py.h"

typedef struct {
    PyObject_HEAD
    HySubject pattern;
} _SubjectObject;

static PyObject *
get_pattern(_SubjectObject *self, void *closure)
{
    if (self->pattern == NULL)
	Py_RETURN_NONE;
    return PyString_FromString(self->pattern);
}

static PyGetSetDef subject_getsetters[] = {
    {"pattern", (getter)get_pattern, NULL, NULL, NULL},
    {NULL}          /* sentinel */
};

static PyObject *
subject_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _SubjectObject *self = (_SubjectObject*)type->tp_alloc(type, 0);
    if (self) {
	self->pattern = NULL;
    }
    return (PyObject*)self;
}

static void
subject_dealloc(_SubjectObject *self)
{
    hy_subject_free(self->pattern);
    Py_TYPE(self)->tp_free(self);
}

static int
subject_init(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    char *pattern = NULL;
    if (!PyArg_ParseTuple(args, "s", &pattern))
	return -1;
    self->pattern = solv_strdup(pattern);
    return 0;
}

static HyForm *
forms_from_list(PyObject *list)
{
    HyForm *forms = NULL;
    int i = 0;
    const int BLOCK_SIZE = 6;
    while (i < PyList_Size(list)) {
        PyObject *form = PyList_GetItem(list, i);
        if (!PyInt_Check(form)) {
            solv_free(forms);
            return NULL;
        }
        forms = solv_extend(forms, i, 1, sizeof(HyForm), BLOCK_SIZE);
        forms[i++] = PyLong_AsLong(form);
    }
    forms = solv_extend(forms, i, 1, sizeof(HyForm), BLOCK_SIZE);
    forms[i] = -1;
    return forms;
}

static HyForm *
forms_from_int(PyObject *num)
{
    HyForm *forms = solv_calloc(2, sizeof(HyForm));
    forms[0] = PyLong_AsLong(num);
    forms[1] = -1;
    return forms;
}

static HyForm *
fill_form(PyObject *o)
{
    HyForm *cforms;
    if (PyList_Check(o))
	cforms = forms_from_list(o);
    else if (PyInt_Check(o))
	cforms = forms_from_int(o);
    if (cforms == NULL) {
	PyErr_SetString(PyExc_TypeError, "Malformed subject forms.");
	return NULL;
    }
    return cforms;
}

/* object methods */

static PyObject *
nevra_possibilities(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *form = NULL;
    char *kwlist[] = { "form", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &form)) {
	return NULL;
    }
    HyForm *cforms = NULL;
    if (form != NULL) {
	cforms = fill_form(form);
	if (cforms == NULL)
	    return NULL;
    }
    HyPossibilities iter = hy_subject_nevra_possibilities(self->pattern,
	cforms);
    solv_free(cforms);
    return possibilitiesToPyObject(iter, NULL);
}

static PyObject *
nevra_possibilities_real(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *sack = NULL;
    HySack csack = NULL;
    int allow_globs = 0;
    int icase = 0;
    int flags = 0;
    PyObject *form = NULL;
    char *kwlist[] = { "sack", "allow_globs", "icase", "form", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|iiO", kwlist,
	&sack_Type, &sack, &allow_globs, &icase, &form))
	return NULL;
    csack = sackFromPyObject(sack);
    if (csack == NULL)
	return NULL;
    HyForm *cforms = NULL;
    if (form != NULL) {
	cforms = fill_form(form);
	if (cforms == NULL)
	    return NULL;
    }
    if (icase)
	flags |= HY_ICASE;
    if (allow_globs)
	flags |= HY_GLOB;

    HyPossibilities iter = hy_subject_nevra_possibilities_real(self->pattern,
    cforms, csack, flags);
    solv_free(cforms);
    return possibilitiesToPyObject(iter, sack);
}

static PyObject *
reldep_possibilities_real(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *sack = NULL;
    HySack csack = NULL;
    int icase = 0;
    int flags = 0;
    char *kwlist[] = { "sack", "icase", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|i", kwlist,
	&sack_Type, &sack, &icase))
	return NULL;
    csack = sackFromPyObject(sack);
    if (csack == NULL)
	return NULL;
    if (icase)
	flags |= HY_ICASE;
    
    HyPossibilities iter = hy_subject_reldep_possibilities_real(self->pattern,
	csack, flags);
    return possibilitiesToPyObject(iter, sack);
}

static struct PyMethodDef subject_methods[] = {
    {"nevra_possibilities", (PyCFunction) nevra_possibilities,
    METH_VARARGS | METH_KEYWORDS, NULL},
    {"nevra_possibilities_real", (PyCFunction) nevra_possibilities_real,
    METH_VARARGS | METH_KEYWORDS, NULL},
    {"reldep_possibilities_real", (PyCFunction) reldep_possibilities_real,
    METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}                      /* sentinel */
};

PyTypeObject subject_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hawkey.Subject",        /*tp_name*/
    sizeof(_SubjectObject),   /*tp_basicsize*/
    0,              /*tp_itemsize*/
    (destructor) subject_dealloc,  /*tp_dealloc*/
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
    "Subject object",     /* tp_doc */
    0,              /* tp_traverse */
    0,              /* tp_clear */
    0,              /* tp_richcompare */
    0,              /* tp_weaklistoffset */
    0,/* tp_iter */
    0,                          /* tp_iternext */
    subject_methods,      /* tp_methods */
    0,              /* tp_members */
    subject_getsetters,       /* tp_getset */
    0,              /* tp_base */
    0,              /* tp_dict */
    0,              /* tp_descr_get */
    0,              /* tp_descr_set */
    0,              /* tp_dictoffset */
    (initproc)subject_init,   /* tp_init */
    0,              /* tp_alloc */
    subject_new,          /* tp_new */
    0,              /* tp_free */
    0,              /* tp_is_gc */
};
