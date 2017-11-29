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
#include "hy-iutil.h"
#include "hy-nevra.h"
#include "dnf-sack.h"
#include "hy-subject.h"
#include "hy-types.h"

// pyhawkey
#include "iutil-py.hpp"
#include "nevra-py.hpp"
#include "possibilities-py.hpp"
#include "pycomp.hpp"
#include "query-py.hpp"
#include "reldep-py.hpp"
#include "sack-py.hpp"
#include "selector-py.hpp"
#include "subject-py.hpp"

typedef struct {
    PyObject_HEAD
    HySubject pattern;
    bool icase;
} _SubjectObject;

static PyObject *
get_icase(_SubjectObject *self, void *closure)
{
    if (self->icase)
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject *
get_pattern(_SubjectObject *self, void *closure)
{
    if (self->pattern == NULL)
        Py_RETURN_NONE;
    return PyUnicode_FromString(self->pattern);
}

static PyGetSetDef subject_getsetters[] = {
    {(char*)"icase", (getter)get_icase, NULL, NULL, NULL},
    {(char*)"pattern", (getter)get_pattern, NULL, NULL, NULL},
    {NULL}          /* sentinel */
};

static PyObject *
subject_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _SubjectObject *self = (_SubjectObject*)type->tp_alloc(type, 0);
    if (self) {
        self->pattern = NULL;
        self->icase = FALSE;
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
    const char * pattern;
    PyObject *icase = NULL;
    const char *kwlist[] = { "pattern", "ignore_case", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|O!", (char**) kwlist, &pattern,
        &PyBool_Type, &icase)) {
        return -1;
    }
    self->pattern = g_strdup(pattern);
    self->icase = icase != NULL && PyObject_IsTrue(icase);
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
            g_free(forms);
            return NULL;
        }
        forms = static_cast<HyForm *>(solv_extend(forms, i, 1, sizeof(HyForm), BLOCK_SIZE));
        forms[i++] = static_cast<HyForm>(PyLong_AsLong(form));
    }
    forms = static_cast<HyForm *>(solv_extend(forms, i, 1, sizeof(HyForm), BLOCK_SIZE));
    forms[i] = _HY_FORM_STOP_;
    return forms;
}

static HyModuleFormEnum *
module_forms_from_list(PyObject *list)
{
    HyModuleFormEnum *forms = NULL;
    int i = 0;
    const int BLOCK_SIZE = 17;
    while (i < PyList_Size(list)) {
        PyObject *form = PyList_GetItem(list, i);
        if (!PyInt_Check(form)) {
            g_free(forms);
            return NULL;
        }
        forms = static_cast<HyModuleFormEnum *>(solv_extend(forms, i, 1, sizeof(HyModuleFormEnum), BLOCK_SIZE));
        forms[i++] = static_cast<HyModuleFormEnum>(PyLong_AsLong(form));
    }
    forms = static_cast<HyModuleFormEnum *>(solv_extend(forms, i, 1, sizeof(HyModuleFormEnum), BLOCK_SIZE));
    forms[i] = _HY_MODULE_FORM_STOP_;
    return forms;
}

static HyForm *
forms_from_int(PyObject *num)
{
    HyForm *forms = g_new0(HyForm, 2);
    forms[0] = static_cast<HyForm>(PyLong_AsLong(num));
    forms[1] = _HY_FORM_STOP_;
    return forms;
}

static HyModuleFormEnum *
module_forms_from_int(PyObject *num)
{
    HyModuleFormEnum *forms = g_new0(HyModuleFormEnum, 2);
    forms[0] = static_cast<HyModuleFormEnum>(PyLong_AsLong(num));
    forms[1] = _HY_MODULE_FORM_STOP_;
    return forms;
}

static HyForm *
fill_form(PyObject *o)
{
    HyForm *cforms = NULL;
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

static HyModuleFormEnum *
fill_module_form(PyObject *o)
{
    HyModuleFormEnum *cforms = NULL;
    if (PyList_Check(o))
        cforms = module_forms_from_list(o);
    else if (PyInt_Check(o))
        cforms = module_forms_from_int(o);
    if (cforms == NULL) {
        PyErr_SetString(PyExc_TypeError, "Malformed subject forms.");
        return NULL;
    }
    return cforms;
}

/* object methods */

static PyObject *
get_nevra_possibilities(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *forms = NULL;
    const char *kwlist[] = { "forms", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", (char**) kwlist, &forms)) {
        return NULL;
    }
    HyForm *cforms = NULL;
    if ((forms != NULL) && (forms != Py_None) &&
        ((!PyList_Check(forms)) || (PyList_Size(forms) > 0))) {
        cforms = fill_form(forms);
        if (cforms == NULL)
            return NULL;
    }
    HyPossibilities iter = hy_subject_nevra_possibilities(self->pattern,
        cforms);
    g_free(cforms);
    return possibilitiesToPyObject(iter, NULL);
}

static PyObject *
nevra_possibilities_real(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *sack = NULL;
    DnfSack *csack = NULL;
    int allow_globs = 0;
    int icase = 0;
    int flags = 0;
    PyObject *form = NULL;
    const char *kwlist[] = { "sack", "allow_globs", "icase", "form", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|iiO", (char**) kwlist,
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
    g_free(cforms);
    return possibilitiesToPyObject(iter, sack);
}

static PyObject *
module_form_possibilities(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *form = NULL;
    const char *kwlist[] = { "form", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", (char**) kwlist, &form)) {
        return NULL;
    }
    HyModuleFormEnum *cforms = NULL;
    if (form != NULL) {
        cforms = fill_module_form(form);
        if (cforms == NULL)
            return NULL;
    }
    HyPossibilities iter = hy_subject_module_form_possibilities(self->pattern,
                                                                cforms);
    g_free(cforms);
    return possibilitiesToPyObject(iter, NULL);
}

static PyObject *
reldep_possibilities_real(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    printf("The function 'reldep_possibilities_real()' in Subject() is deprecated and will be "
           "removed on 2018-01-01\n");
    PyObject *sack = NULL;
    DnfSack *csack = NULL;
    int icase = 0;
    int flags = 0;
    const char *kwlist[] = { "sack", "icase", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|i", (char**) kwlist,
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

static PyObject *
get_best_parser(_SubjectObject *self, PyObject *args, PyObject *kwds, HyNevra *nevra)
{
    PyObject *sack;
    DnfSack *csack;
    PyObject *forms = NULL;
    PyObject *with_nevra = NULL;
    PyObject *with_provides = NULL;
    PyObject *with_filenames = NULL;
    const char *kwlist[] = {"sack", "with_nevra", "with_provides", "with_filenames", "forms", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!O!O!O", (char**) kwlist, &sack_Type, &sack,
        &PyBool_Type, &with_nevra, &PyBool_Type, &with_provides,
        &PyBool_Type, &with_filenames, &forms)) {
        return NULL;
    }
    HyForm *cforms = NULL;
    if ((forms != NULL) && (forms != Py_None) &&
        ((!PyList_Check(forms)) || (PyList_Size(forms) > 0))) {
        cforms = fill_form(forms);
        if (cforms == NULL)
            return NULL;
    }
    gboolean c_with_nevra = with_nevra == NULL || PyObject_IsTrue(with_nevra);
    gboolean c_with_provides = with_provides == NULL || PyObject_IsTrue(with_provides);
    gboolean c_with_filenames = with_filenames == NULL || PyObject_IsTrue(with_filenames);
    csack = sackFromPyObject(sack);
    HyQuery query = hy_subject_get_best_solution(self->pattern, csack, cforms, nevra, self->icase,
                                                 c_with_nevra, c_with_provides, c_with_filenames);

    return queryToPyObject(query, sack, &query_Type);
}

static PyObject *
get_best_query(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    HyNevra nevra = NULL;

    PyObject *py_query = get_best_parser(self, args, kwds, &nevra);
    if (nevra != NULL) {
        hy_nevra_free(nevra);
    }
    Py_XINCREF(py_query);
    return py_query;
}

static PyObject *
get_best_selector(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *sack;
    PyObject *forms = NULL;
    PyObject *obsoletes = NULL;
    char *reponame = NULL;
    PyObject *reports = NULL;
    const char *kwlist[] = {"sack", "forms", "obsoletes", "reponame", "reports", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|OO!zO!", (char**) kwlist, &sack_Type, &sack,
        &forms, &PyBool_Type, &obsoletes, &reponame, &PyBool_Type, &reports)) {
        return NULL;
    }
    HyForm *cforms = NULL;
    if ((forms != NULL) && (forms != Py_None) &&
        ((!PyList_Check(forms)) || (PyList_Size(forms) > 0))) {
        cforms = fill_form(forms);
        if (cforms == NULL)
            return NULL;
    }

    bool c_obsoletes = obsoletes == NULL || PyObject_IsTrue(obsoletes);
    bool c_reports = reports != NULL && PyObject_IsTrue(reports);
    if (c_reports) {
        printf("The attribute 'reports' in get_best_selector() is deprecated and not used any more."
               " This attribute will be removed on 2018-01-01\n");
    }
    DnfSack *csack = sackFromPyObject(sack);
    HySelector c_selector = hy_subject_get_best_sltr(self->pattern, csack, cforms, c_obsoletes,
                                                     reponame);
    PyObject *selector = SelectorToPyObject(c_selector, sack);
    Py_INCREF(selector);
    return selector;
}

static PyObject *
get_best_solution(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    HyNevra nevra = NULL;

    PyObject *q = get_best_parser(self, args, kwds, &nevra);
    if (q == NULL)
        return NULL;
    PyObject *ret_dict = PyDict_New();
    PyDict_SetItem(ret_dict, PyString_FromString("query"), q);
    Py_DECREF(q);
    if (nevra != NULL) {
        PyObject *n = nevraToPyObject(nevra);
        PyDict_SetItem(ret_dict, PyString_FromString("nevra"), n);
        Py_DECREF(n);
    }
    else
        PyDict_SetItem(ret_dict, PyString_FromString("nevra"), Py_None);

    return ret_dict;
}

static struct PyMethodDef subject_methods[] = {
    {"get_nevra_possibilities", (PyCFunction) get_nevra_possibilities,
    METH_VARARGS | METH_KEYWORDS,
    "get_nevra_possibilities(self, forms=None)\n"
    "# :api\n"
    "forms: list of hawkey NEVRA forms like [hawkey.FORM_NEVRA, hawkey.FORM_NEVR]\n"
    "return: object with every possible nevra. Each possible nevra is represented by Class "
    "NEVRA object (libdnf) that have attributes name, epoch, version, release, arch"},
    {"nevra_possibilities_real", (PyCFunction) nevra_possibilities_real,
    METH_VARARGS | METH_KEYWORDS, "DEPRECATED!"},
    {"module_form_possibilities", (PyCFunction) module_form_possibilities,
    METH_VARARGS | METH_KEYWORDS, NULL},
    {"reldep_possibilities_real", (PyCFunction) reldep_possibilities_real,
    METH_VARARGS | METH_KEYWORDS, "DEPRECATED!"},
    {"get_best_query", (PyCFunction) get_best_query,
    METH_VARARGS | METH_KEYWORDS,
    "get_best_query(self, sack, with_nevra=True, with_provides=True, with_filenames=True,\n"
    "    forms=None)\n"
    "# :api\n"
    "Try to find first real solution for subject if it is NEVRA, provide or file name\n"
    "return: query"},
    {"get_best_selector", (PyCFunction) get_best_selector, METH_VARARGS | METH_KEYWORDS,
    "get_best_selector(self, sack, forms=None, obsoletes=True, reponame=None, reports=False)\n"
    "# :api"},
    {"get_best_solution", (PyCFunction) get_best_solution,
    METH_VARARGS | METH_KEYWORDS,
    "get_best_solution(self, sack, with_nevra=True, with_provides=True, with_filenames=True,\n"
    "    forms=None)\n"
    "# :api\n"
    "Try to find first real solution for subject if it is NEVRA, provide or file name\n"
    "return: dict with keys nevra and query"},
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
