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

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdio.h>

#include <solv/util.h>

#include "dnf-advisory.h"
#include "hy-iutil.h"
#include "hy-package.h"
#include "hy-package-private.hpp"
#include "dnf-package.h"
#include "dnf-reldep.h"
#include "dnf-types.h"
#include "libdnf/sack/packageset.hpp"

#include "exception-py.hpp"
#include "iutil-py.hpp"
#include "package-py.hpp"
#include "packagedelta-py.hpp"
#include "sack-py.hpp"
#include "pycomp.hpp"
#include "libdnf/repo/solvable/DependencyContainer.hpp"

typedef struct {
    PyObject_HEAD
    DnfPackage *package;
    PyObject *sack;
} _PackageObject;

long package_hash(_PackageObject *self);

DnfPackage *
packageFromPyObject(PyObject *o)
{
    if (!PyType_IsSubtype(o->ob_type, &package_Type)) {
        PyErr_SetString(PyExc_TypeError, "Expected a Package object.");
        return NULL;
    }
    return ((_PackageObject *)o)->package;
}

int
package_converter(PyObject *o, DnfPackage **pkg_ptr)
{
    DnfPackage *pkg = packageFromPyObject(o);
    if (pkg == NULL)
        return 0;
    *pkg_ptr = pkg;
    return 1;
}

/* functions on the type */

static PyObject *
package_new(PyTypeObject *type, PyObject *args, PyObject *kwds) try
{
    _PackageObject *self = (_PackageObject*)type->tp_alloc(type, 0);
    if (self) {
        self->sack = NULL;
        self->package = NULL;
    }
    return (PyObject*)self;
} CATCH_TO_PYTHON

static void
package_dealloc(_PackageObject *self)
{
    if (self->package)
        g_object_unref(self->package);

    Py_XDECREF(self->sack);
    Py_TYPE(self)->tp_free(self);
}

static int
package_init(_PackageObject *self, PyObject *args, PyObject *kwds) try
{
    Id id;
    PyObject *sack;
    DnfSack *csack;

    if (!PyArg_ParseTuple(args, "(O!i)", &sack_Type, &sack, &id))
        return -1;
    csack = sackFromPyObject(sack);
    if (csack == NULL)
        return -1;
    self->sack = sack;
    Py_INCREF(self->sack);
    self->package = dnf_package_new(csack, id);
    return 0;
} CATCH_TO_PYTHON_INT

static PyObject *
package_py_richcompare(PyObject *self, PyObject *other, int op) try
{
    PyObject *v;
    DnfPackage *self_package, *other_package;

    if (!package_converter(self, &self_package) ||
        !package_converter(other, &other_package)) {
        if(PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_TypeError))
            PyErr_Clear();
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    long result = dnf_package_cmp(self_package, other_package);

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
} CATCH_TO_PYTHON

static PyObject *
package_repr(_PackageObject *self) try
{
    DnfPackage *pkg = self->package;
    const char *nevra = dnf_package_get_nevra(pkg);
    PyObject *repr;

    repr = PyString_FromFormat("<hawkey.Package object id %ld, %s, %s>",
                               package_hash(self), nevra,
                               dnf_package_get_reponame(pkg));
    return repr;
} CATCH_TO_PYTHON

static PyObject *
package_str(_PackageObject *self) try
{
    const char *cstr = dnf_package_get_nevra(self->package);
    PyObject *ret = PyString_FromString(cstr);
    return ret;
} CATCH_TO_PYTHON

long package_hash(_PackageObject *self) try
{
    return dnf_package_get_id(self->package);
} CATCH_TO_PYTHON_INT

/* getsetters */

static PyObject *
get_bool(_PackageObject *self, void *closure) try
{
    unsigned long (*func)(DnfPackage*);
    func = (unsigned long (*)(DnfPackage*))closure;
    return PyBool_FromLong(func(self->package));
} CATCH_TO_PYTHON

static PyObject *
get_num(_PackageObject *self, void *closure) try
{
    guint64 (*func)(DnfPackage*);
    func = (guint64 (*)(DnfPackage*))closure;
    return PyLong_FromUnsignedLongLong(func(self->package));
} CATCH_TO_PYTHON

static PyObject *
get_reldep(_PackageObject *self, void *closure) try
{
    DnfReldepList *(*func)(DnfPackage*) = (DnfReldepList *(*)(DnfPackage*))closure;
    std::unique_ptr<DnfReldepList> reldeplist(func(self->package));
    assert(reldeplist);
    PyObject *list = reldeplist_to_pylist(reldeplist.get(), self->sack);

    return list;
} CATCH_TO_PYTHON

static PyObject *
get_str(_PackageObject *self, void *closure) try
{
    const char *(*func)(DnfPackage*);
    const char *cstr;

    func = (const char *(*)(DnfPackage*))closure;
    cstr = func(self->package);
    if (cstr == NULL)
        Py_RETURN_NONE;
    return PyUnicode_FromString(cstr);
} CATCH_TO_PYTHON

static PyObject *
get_str_array(_PackageObject *self, void *closure) try
{
    gchar ** (*func)(DnfPackage*);
    gchar ** strv;

    func = (gchar **(*)(DnfPackage*))closure;
    strv = func(self->package);
    PyObject *list = strlist_to_pylist((const char **)strv);
    g_strfreev(strv);

    return list;
} CATCH_TO_PYTHON

static PyObject *
get_chksum(_PackageObject *self, void *closure) try
{
    HyChecksum *(*func)(DnfPackage*, int *);
    int type;
    HyChecksum *cs;

    func = (HyChecksum *(*)(DnfPackage*, int *))closure;
    cs = func(self->package, &type);
    if (cs == 0) {
        Py_RETURN_NONE;
    }

    PyObject *res;
    int checksum_length = checksum_type2length(type);

#if PY_MAJOR_VERSION < 3
    res = Py_BuildValue("is#", type, cs, checksum_length);
#else
    res = Py_BuildValue("iy#", type, cs, (Py_ssize_t)checksum_length);
#endif

    return res;
} CATCH_TO_PYTHON

static PyObject *
get_changelogs(_PackageObject *self, void *closure) try
{
    return changelogslist_to_pylist(dnf_package_get_changelogs(self->package));
} CATCH_TO_PYTHON

static PyObject *
get_local_baseurl(_PackageObject *self) try
{
    g_autoptr(GError) error = nullptr;
    gchar *local_baseurl = dnf_package_get_local_baseurl(self->package, &error);

    if (error != nullptr) {
        op_error2exc(error);
        return nullptr;
    }

    if (local_baseurl == nullptr) {
        return nullptr;
    }

    PyObject *ret = PyString_FromString(local_baseurl);
    g_free(local_baseurl);
    return ret;
} CATCH_TO_PYTHON

static PyGetSetDef package_getsetters[] = {
    {(char*)"baseurl",        (getter)get_str, NULL, NULL,
     (void *)dnf_package_get_baseurl},
    {(char*)"files",        (getter)get_str_array, NULL, NULL,
     (void *)dnf_package_get_files},
    {(char*)"changelogs", (getter)get_changelogs, NULL, NULL, NULL},
    {(char*)"hdr_end", (getter)get_num, NULL, NULL, (void *)dnf_package_get_hdr_end},
    {(char*)"location",  (getter)get_str, NULL, NULL,
     (void *)dnf_package_get_location},
    {(char*)"sourcerpm",  (getter)get_str, NULL, NULL,
     (void *)dnf_package_get_sourcerpm},
    {(char*)"version",  (getter)get_str, NULL, NULL,
     (void *)dnf_package_get_version},
    {(char*)"release",  (getter)get_str, NULL, NULL,
     (void *)dnf_package_get_release},
    {(char*)"name", (getter)get_str, NULL, NULL, (void *)dnf_package_get_name},
    {(char*)"arch", (getter)get_str, NULL, NULL, (void *)dnf_package_get_arch},
    {(char*)"hdr_chksum", (getter)get_chksum, NULL, NULL,
     (void *)dnf_package_get_hdr_chksum},
    {(char*)"chksum", (getter)get_chksum, NULL, NULL, (void *)dnf_package_get_chksum},
    {(char*)"description", (getter)get_str, NULL, NULL,
     (void *)dnf_package_get_description},
    {(char*)"evr",  (getter)get_str, NULL, NULL, (void *)dnf_package_get_evr},
    {(char*)"group",  (getter)get_str, NULL, NULL, (void *)dnf_package_get_group},
    {(char*)"license", (getter)get_str, NULL, NULL, (void *)dnf_package_get_license},
    {(char*)"packager",  (getter)get_str, NULL, NULL, (void *)dnf_package_get_packager},
    {(char*)"reponame",  (getter)get_str, NULL, NULL, (void *)dnf_package_get_reponame},
    {(char*)"summary",  (getter)get_str, NULL, NULL, (void *)dnf_package_get_summary},
    {(char*)"url",  (getter)get_str, NULL, NULL, (void *)dnf_package_get_url},
    {(char*)"downloadsize", (getter)get_num, NULL, NULL,
     (void *)dnf_package_get_downloadsize},
    {(char*)"epoch", (getter)get_num, NULL, NULL, (void *)dnf_package_get_epoch},
    {(char*)"installsize", (getter)get_num, NULL, NULL,
     (void *)dnf_package_get_installsize},
    {(char*)"buildtime", (getter)get_num, NULL, NULL, (void *)dnf_package_get_buildtime},
    {(char*)"installtime", (getter)get_num, NULL, NULL,
     (void *)dnf_package_get_installtime},
    {(char*)"installed", (getter)get_bool, NULL, NULL, (void *)dnf_package_installed},
    {(char*)"medianr", (getter)get_num, NULL, NULL, (void *)dnf_package_get_medianr},
    {(char*)"rpmdbid", (getter)get_num, NULL, NULL, (void *)dnf_package_get_rpmdbid},
    {(char*)"size", (getter)get_num, NULL, NULL, (void *)dnf_package_get_size},
    {(char*)"conflicts",  (getter)get_reldep, NULL, NULL,
     (void *)dnf_package_get_conflicts},
    {(char*)"enhances",  (getter)get_reldep, NULL, NULL,
     (void *)dnf_package_get_enhances},
    {(char*)"obsoletes",  (getter)get_reldep, NULL, NULL,
     (void *)dnf_package_get_obsoletes},
    {(char*)"requires_pre",  (getter)get_reldep, NULL, NULL,
     (void *)dnf_package_get_requires_pre},
    {(char*)"provides",  (getter)get_reldep, NULL, NULL,
     (void *)dnf_package_get_provides},
    {(char*)"recommends",  (getter)get_reldep, NULL, NULL,
     (void *)dnf_package_get_recommends},
    {(char*)"requires",  (getter)get_reldep, NULL, NULL,
     (void *)dnf_package_get_requires},
    {(char*)"suggests",  (getter)get_reldep, NULL, NULL,
     (void *)dnf_package_get_suggests},
    {(char*)"supplements",  (getter)get_reldep, NULL, NULL,
     (void *)dnf_package_get_supplements},
    {(char*)"prereq_ignoreinst",  (getter)get_reldep, NULL, NULL,
     (void *)dnf_package_get_prereq_ignoreinst},
    {(char*)"regular_requires",  (getter)get_reldep, NULL, NULL,
     (void *)dnf_package_get_regular_requires},
    {NULL}                        /* sentinel */
};

/* object methods */

static PyObject *
evr_cmp(_PackageObject *self, PyObject *other) try
{
    DnfPackage *pkg2 = packageFromPyObject(other);
    if (pkg2 == NULL)
        return NULL;
    return PyLong_FromLong(dnf_package_evr_cmp(self->package, pkg2));
} CATCH_TO_PYTHON

static PyObject *
get_delta_from_evr(_PackageObject *self, PyObject *evr_str) try
{
    PycompString evr(evr_str);
    if (!evr.getCString())
        return NULL;
    DnfPackageDelta *delta_c = dnf_package_get_delta_from_evr(self->package, evr.getCString());
    if (delta_c)
        return packageDeltaToPyObject(delta_c);
    Py_RETURN_NONE;
} CATCH_TO_PYTHON

static PyObject *
is_in_active_module(_PackageObject *self, PyObject *unused) try
{
    DnfSack * csack = sackFromPyObject(self->sack);
    std::unique_ptr<DnfPackageSet> includes(dnf_sack_get_module_includes(csack));
    if (!includes) {
        Py_RETURN_FALSE;
    }
    if (includes->has(dnf_package_get_id(self->package))) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
} CATCH_TO_PYTHON

static PyObject *
get_advisories(_PackageObject *self, PyObject *args) try
{
    int cmp_type;
    GPtrArray *advisories;
    PyObject *list;

    if (!PyArg_ParseTuple(args, "i", &cmp_type))
        return NULL;

    advisories = dnf_package_get_advisories(self->package, cmp_type);
    list = advisorylist_to_pylist(advisories, self->sack);
    g_ptr_array_unref(advisories);

    return list;
} CATCH_TO_PYTHON


static struct PyMethodDef package_methods[] = {
    {"evr_cmp", (PyCFunction)evr_cmp, METH_O, NULL},
    {"get_delta_from_evr", (PyCFunction)get_delta_from_evr, METH_O, NULL},
    {"get_advisories", (PyCFunction)get_advisories, METH_VARARGS, NULL},
    {"_is_in_active_module", (PyCFunction)is_in_active_module, METH_NOARGS, NULL},
    {"get_local_baseurl", (PyCFunction)get_local_baseurl, METH_NOARGS, NULL},
    {NULL}                      /* sentinel */
};

PyTypeObject package_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hawkey.Package",                /*tp_name*/
    sizeof(_PackageObject),        /*tp_basicsize*/
    0,                                /*tp_itemsize*/
    (destructor) package_dealloc, /*tp_dealloc*/
    0,                                /*tp_print*/
    0,                                /*tp_getattr*/
    0,                                /*tp_setattr*/
    0,                                /*tp_compare*/
    (reprfunc)package_repr,        /*tp_repr*/
    0,                                /*tp_as_number*/
    0,                                /*tp_as_sequence*/
    0,                                /*tp_as_mapping*/
    (hashfunc)package_hash,        /*tp_hash */
    0,                                /*tp_call*/
    (reprfunc)package_str,        /*tp_str*/
    0,                                /*tp_getattro*/
    0,                                /*tp_setattro*/
    0,                                /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "Package object",                /* tp_doc */
    0,                                /* tp_traverse */
    0,                                /* tp_clear */
    (richcmpfunc) package_py_richcompare,        /* tp_richcompare */
    0,                                /* tp_weaklistoffset */
    0,                                /* tp_iter */
    0,                                 /* tp_iternext */
    package_methods,                /* tp_methods */
    0,                                /* tp_members */
    package_getsetters,                /* tp_getset */
    0,                                /* tp_base */
    0,                                /* tp_dict */
    0,                                /* tp_descr_get */
    0,                                /* tp_descr_set */
    0,                                /* tp_dictoffset */
    (initproc)package_init,        /* tp_init */
    0,                                /* tp_alloc */
    package_new,                /* tp_new */
    0,                                /* tp_free */
    0,                                /* tp_is_gc */
};
