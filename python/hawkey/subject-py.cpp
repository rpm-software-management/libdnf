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

#include <vector>

// hawkey
#include "hy-iutil.h"
#include "nevra.hpp"
#include "nsvcap.hpp"
#include "dnf-sack.h"
#include "hy-subject.h"
#include "hy-types.h"

// pyhawkey
#include "iutil-py.hpp"
#include "nevra-py.hpp"
#include "nsvcap-py.hpp"
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
    PyObject *py_pattern;
    PyObject *icase = NULL;
    const char *kwlist[] = { "pattern", "ignore_case", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O!", (char**) kwlist,
        &py_pattern, &PyBool_Type, &icase)) {
        return -1;
    }
    self->icase = icase != NULL && PyObject_IsTrue(icase);
    PycompString pattern(py_pattern);
    if (!pattern.getCString())
        return -1;
    self->pattern = g_strdup(pattern.getCString());
    return 0;
}

template<typename T, T last_element>
static std::vector<T>
fill_form(PyObject *o)
{
    if (PyList_Check(o)) {
        std::vector<T> cforms;
        cforms.reserve(PyList_Size(o) + 1);
        bool error{false};
        for (Py_ssize_t i = 0; i < PyList_Size(o); ++i) {
            PyObject *form = PyList_GetItem(o, i);
            if (!PyInt_Check(form)) {
                error = true;
                break;
            }
            cforms.push_back(static_cast<T>(PyLong_AsLong(form)));
        }
        if (!error) {
            cforms.push_back(last_element);
            return cforms;
        }
    }
    else if (PyInt_Check(o))
        return {static_cast<T>(PyLong_AsLong(o)), last_element};

    PyErr_SetString(PyExc_TypeError, "Malformed subject forms.");
    return {};
}

/* object methods */

static bool
addNevraToPyList(PyObject * pyList, libdnf::Nevra & nevraObj)
{
    auto cNevra = new libdnf::Nevra(std::move(nevraObj));
    UniquePtrPyObject nevra(nevraToPyObject(cNevra));
    if (!nevra) {
        delete cNevra;
        return false;
    }
    int rc = PyList_Append(pyList, nevra.get());
    if (rc == -1)
        return false;
    return true;
}

static PyObject *
get_nevra_possibilities(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *forms = NULL;
    const char *kwlist[] = { "forms", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", (char**) kwlist, &forms)) {
        return NULL;
    }

    UniquePtrPyObject list(PyList_New(0));
    if (!list)
        return NULL;
    libdnf::Nevra nevraObj;
    if (forms && forms != Py_None) {
        if (PyInt_Check(forms)) {
            if (nevraObj.parse(self->pattern, static_cast<HyForm>(PyLong_AsLong(forms)))) {
                if (!addNevraToPyList(list.get(), nevraObj))
                    return NULL;
            }
            return list.release();
        }
        else if (PyList_Check(forms)) {
            bool error = false;
            for (Py_ssize_t i = 0; i < PyList_Size(forms); ++i) {
                PyObject *form = PyList_GetItem(forms, i);
                if (!PyInt_Check(form)) {
                    error = true;
                    break;
                }
                if (nevraObj.parse(self->pattern, static_cast<HyForm>(PyLong_AsLong(form)))) {
                    if (!addNevraToPyList(list.get(), nevraObj))
                        return NULL;
                }
            }
            if (!error) 
                return list.release();
        }
        PyErr_SetString(PyExc_TypeError, "Malformed subject forms.");
        return NULL;
    } else {
        for (std::size_t i = 0; HY_FORMS_MOST_SPEC[i] != _HY_FORM_STOP_; ++i) {
            if (nevraObj.parse(self->pattern, HY_FORMS_MOST_SPEC[i])) {
                if (!addNevraToPyList(list.get(), nevraObj))
                    return NULL;
            }
        }
    }
    return list.release();
}

static bool
addNsvcapToPyList(PyObject * pyList, libdnf::Nsvcap & nevraObj)
{
    auto cNsvcap = new libdnf::Nsvcap(std::move(nevraObj));
    UniquePtrPyObject nsvcap(nsvcapToPyObject(cNsvcap));
    if (!nsvcap) {
        delete cNsvcap;
        return false;
    }
    int rc = PyList_Append(pyList, nsvcap.get());
    if (rc == -1)
        return false;
    return true;
}

static PyObject *
nsvcap_possibilities(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *forms = NULL;
    const char *kwlist[] = { "form", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", (char**) kwlist, &forms)) {
        return NULL;
    }

    UniquePtrPyObject list(PyList_New(0));
    if (!list)
        return NULL;
    libdnf::Nsvcap nsvcapObj;
    if (forms && forms != Py_None) {
        if (PyInt_Check(forms)) {
            if (nsvcapObj.parse(self->pattern, static_cast<HyModuleForm>(PyLong_AsLong(forms)))) {
                if (!addNsvcapToPyList(list.get(), nsvcapObj))
                    return NULL;
            }
            return list.release();
        }
        else if (PyList_Check(forms)) {
            bool error = false;
            for (Py_ssize_t i = 0; i < PyList_Size(forms); ++i) {
                PyObject *form = PyList_GetItem(forms, i);
                if (!PyInt_Check(form)) {
                    error = true;
                    break;
                }
                if (nsvcapObj.parse(self->pattern, static_cast<HyModuleForm>(PyLong_AsLong(form)))) {
                    if (!addNsvcapToPyList(list.get(), nsvcapObj))
                        return NULL;
                }
            }
            if (!error) 
                return list.release();
        }
        PyErr_SetString(PyExc_TypeError, "Malformed subject forms.");
        return NULL;
    } else {
        for (std::size_t i = 0; HY_FORMS_MOST_SPEC[i] != _HY_FORM_STOP_; ++i) {
            if (nsvcapObj.parse(self->pattern, HY_MODULE_FORMS_MOST_SPEC[i])) {
                if (!addNsvcapToPyList(list.get(), nsvcapObj))
                    return NULL;
            }
        }
    }
    return list.release();

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
    PyObject *with_src = NULL;
    const char *kwlist[] = {"sack", "with_nevra", "with_provides", "with_filenames", "forms",
        "with_src", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!O!O!O!O", (char**) kwlist, &sack_Type, &sack,
        &PyBool_Type, &with_nevra, &PyBool_Type, &with_provides,
        &PyBool_Type, &with_filenames, &forms, &PyBool_Type, &with_src)) {
        return NULL;
    }
    std::vector<HyForm> cforms;
    if ((forms != NULL) && (forms != Py_None) &&
        ((!PyList_Check(forms)) || (PyList_Size(forms) > 0))) {
        cforms = fill_form<HyForm, _HY_FORM_STOP_>(forms);
        if (cforms.empty())
            return NULL;
    }
    gboolean c_with_nevra = with_nevra == NULL || PyObject_IsTrue(with_nevra);
    gboolean c_with_provides = with_provides == NULL || PyObject_IsTrue(with_provides);
    gboolean c_with_filenames = with_filenames == NULL || PyObject_IsTrue(with_filenames);
    gboolean c_with_src = with_src == NULL || PyObject_IsTrue(with_src);
    csack = sackFromPyObject(sack);
    HyQuery query = hy_subject_get_best_solution(self->pattern, csack,
        cforms.empty() ? NULL : cforms.data(), nevra, self->icase, c_with_nevra,
        c_with_provides, c_with_filenames, c_with_src);

    return queryToPyObject(query, sack, &query_Type);
}

static PyObject *
get_best_query(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    HyNevra nevra{nullptr};
    PyObject *py_query = get_best_parser(self, args, kwds, &nevra);
    delete nevra;
    return py_query;
}

static PyObject *
get_best_selector(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *sack;
    PyObject *forms = NULL;
    PyObject *obsoletes = NULL;
    char *reponame = NULL;
    const char *kwlist[] = {"sack", "forms", "obsoletes", "reponame", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|OO!z", (char**) kwlist, &sack_Type, &sack,
        &forms, &PyBool_Type, &obsoletes, &reponame)) {
        return NULL;
    }
    std::vector<HyForm> cforms;
    if ((forms != NULL) && (forms != Py_None) &&
        ((!PyList_Check(forms)) || (PyList_Size(forms) > 0))) {
        cforms = fill_form<HyForm, _HY_FORM_STOP_>(forms);
        if (cforms.empty())
            return NULL;
    }

    bool c_obsoletes = obsoletes == NULL || PyObject_IsTrue(obsoletes);
    DnfSack *csack = sackFromPyObject(sack);
    HySelector c_selector = hy_subject_get_best_selector(self->pattern, csack,
         cforms.empty() ? NULL : cforms.data(), c_obsoletes, reponame);
    PyObject *selector = SelectorToPyObject(c_selector, sack);
    return selector;
}

static PyObject *
get_best_solution(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    HyNevra nevra{nullptr};

    UniquePtrPyObject q(get_best_parser(self, args, kwds, &nevra));
    if (!q)
        return NULL;
    PyObject *ret_dict = PyDict_New();
    PyDict_SetItem(ret_dict, PyString_FromString("query"), q.get());
    if (nevra) {
        UniquePtrPyObject n(nevraToPyObject(nevra));
        PyDict_SetItem(ret_dict, PyString_FromString("nevra"), n.get());
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
    {"nsvcap_possibilities", (PyCFunction) nsvcap_possibilities,
    METH_VARARGS | METH_KEYWORDS, NULL},
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
