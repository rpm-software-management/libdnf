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
#include <stdio.h>

// libsolv
#include <solv/util.h>

// hawkey
#include "src/advisory.h"
#include "src/iutil.h"
#include "src/package_internal.h"
#include "src/packagelist.h"
#include "src/reldep.h"
#include "src/sack_internal.h"

// pyhawkey
#include "iutil-py.h"
#include "package-py.h"
#include "packagedelta-py.h"
#include "sack-py.h"

#include "pycomp.h"

typedef struct {
    PyObject_HEAD
    HyPackage package;
    PyObject *sack;
} _PackageObject;

long package_hash(_PackageObject *self);

HyPackage
packageFromPyObject(PyObject *o)
{
    if (!PyType_IsSubtype(o->ob_type, &package_Type)) {
	PyErr_SetString(PyExc_TypeError, "Expected a Package object.");
	return NULL;
    }
    return ((_PackageObject *)o)->package;
}

int
package_converter(PyObject *o, HyPackage *pkg_ptr)
{
    HyPackage pkg = packageFromPyObject(o);
    if (pkg == NULL)
	return 0;
    *pkg_ptr = pkg;
    return 1;
}

/* functions on the type */

static PyObject *
package_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _PackageObject *self = (_PackageObject*)type->tp_alloc(type, 0);
    if (self) {
	self->sack = NULL;
	self->package = NULL;
    }
    return (PyObject*)self;
}

static void
package_dealloc(_PackageObject *self)
{
    if (self->package)
	hy_package_free(self->package);

    Py_XDECREF(self->sack);
    Py_TYPE(self)->tp_free(self);
}

static int
package_init(_PackageObject *self, PyObject *args, PyObject *kwds)
{
    Id id;
    PyObject *sack;
    HySack csack;

    if (!PyArg_ParseTuple(args, "(O!i)", &sack_Type, &sack, &id))
	return -1;
    csack = sackFromPyObject(sack);
    if (csack == NULL)
	return -1;
    self->sack = sack;
    Py_INCREF(self->sack);
    self->package = package_create(csack, id);
    return 0;
}

PyObject *
package_py_richcompare(PyObject *self, PyObject *other, int op)
{
    PyObject *v;
    HyPackage self_package, other_package;

    if (!package_converter(self, &self_package) ||
        !package_converter(other, &other_package)) {
        if(PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_TypeError))
            PyErr_Clear();
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    long result = hy_package_cmp(self_package, other_package);

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
}

static PyObject *
package_repr(_PackageObject *self)
{
    HyPackage pkg = self->package;
    char *nevra = hy_package_get_nevra(pkg);
    PyObject *repr;

    repr = PyString_FromFormat("<hawkey.Package object id %ld, %s, %s>",
			       package_hash(self), nevra,
			       hy_package_get_reponame(pkg));
    solv_free(nevra);
    return repr;
}

static PyObject *
package_str(_PackageObject *self)
{
    char *cstr = hy_package_get_nevra(self->package);
    PyObject *ret = PyString_FromString(cstr);
    solv_free(cstr);
    return ret;
}

long package_hash(_PackageObject *self)
{
    return package_id(self->package);
}

/* getsetters */

static PyObject *
get_bool(_PackageObject *self, void *closure)
{
    unsigned long (*func)(HyPackage);
    func = (unsigned long (*)(HyPackage))closure;
    return PyBool_FromLong(func(self->package));
}

static PyObject *
get_num(_PackageObject *self, void *closure)
{
    unsigned long long (*func)(HyPackage);
    func = (unsigned long long (*)(HyPackage))closure;
    return PyLong_FromUnsignedLongLong(func(self->package));
}

static PyObject *
get_reldep(_PackageObject *self, void *closure)
{
    HyReldepList (*func)(HyPackage) = (HyReldepList (*)(HyPackage))closure;
    HyReldepList reldeplist = func(self->package);
    assert(reldeplist);
    PyObject *list = reldeplist_to_pylist(reldeplist, self->sack);
    hy_reldeplist_free(reldeplist);

    return list;
}

static PyObject *
get_str(_PackageObject *self, void *closure)
{
    const char *(*func)(HyPackage);
    const char *cstr;

    func = (const char *(*)(HyPackage))closure;
    cstr = func(self->package);
    if (cstr == NULL)
	Py_RETURN_NONE;
    return PyUnicode_FromString(cstr);
}

static PyObject *
get_str_alloced(_PackageObject *self, void *closure)
{
    char *(*func)(HyPackage);
    char *cstr;
    PyObject *ret;

    func = (char *(*)(HyPackage))closure;
    cstr = func(self->package);
    if (cstr == NULL)
	Py_RETURN_NONE;
    ret = PyUnicode_FromString(cstr);
    solv_free(cstr);
    return ret;
}

static PyObject *
get_str_array(_PackageObject *self, void *closure)
{
    gchar ** (*func)(HyPackage);
    gchar ** strv;

    func = (gchar **(*)(HyPackage))closure;
    strv = func(self->package);
    PyObject *list = strlist_to_pylist((const char **)strv);
    g_strfreev(strv);

    return list;
}

static PyObject *
get_chksum(_PackageObject *self, void *closure)
{
    HyChecksum *(*func)(HyPackage, int *);
    int type;
    HyChecksum *cs;

    func = (HyChecksum *(*)(HyPackage, int *))closure;
    cs = func(self->package, &type);
    if (cs == 0) {
	PyErr_SetString(PyExc_AttributeError, "No such checksum.");
	return NULL;
    }

    PyObject *res;
    int checksum_length = checksum_type2length(type);

#if PY_MAJOR_VERSION < 3
    res = Py_BuildValue("is#", type, cs, checksum_length);
#else
    res = Py_BuildValue("iy#", type, cs, checksum_length);
#endif

    return res;
}

static PyGetSetDef package_getsetters[] = {
    {"baseurl",	(getter)get_str, NULL, NULL,
     (void *)hy_package_get_baseurl},
    {"files",	(getter)get_str_array, NULL, NULL,
     (void *)hy_package_get_files},
    {"hdr_end", (getter)get_num, NULL, NULL, (void *)hy_package_get_hdr_end},
    {"location",  (getter)get_str_alloced, NULL, NULL,
     (void *)hy_package_get_location},
    {"sourcerpm",  (getter)get_str_alloced, NULL, NULL,
     (void *)hy_package_get_sourcerpm},
    {"version",  (getter)get_str_alloced, NULL, NULL,
     (void *)hy_package_get_version},
    {"release",  (getter)get_str_alloced, NULL, NULL,
     (void *)hy_package_get_release},
    {"name", (getter)get_str, NULL, NULL, (void *)hy_package_get_name},
    {"arch", (getter)get_str, NULL, NULL, (void *)hy_package_get_arch},
    {"hdr_chksum", (getter)get_chksum, NULL, NULL,
     (void *)hy_package_get_hdr_chksum},
    {"chksum", (getter)get_chksum, NULL, NULL, (void *)hy_package_get_chksum},
    {"description", (getter)get_str, NULL, NULL,
     (void *)hy_package_get_description},
    {"evr",  (getter)get_str, NULL, NULL, (void *)hy_package_get_evr},
    {"license", (getter)get_str, NULL, NULL, (void *)hy_package_get_license},
    {"packager",  (getter)get_str, NULL, NULL, (void *)hy_package_get_packager},
    {"reponame",  (getter)get_str, NULL, NULL, (void *)hy_package_get_reponame},
    {"summary",  (getter)get_str, NULL, NULL, (void *)hy_package_get_summary},
    {"url",  (getter)get_str, NULL, NULL, (void *)hy_package_get_url},
    {"downloadsize", (getter)get_num, NULL, NULL,
     (void *)hy_package_get_downloadsize},
    {"epoch", (getter)get_num, NULL, NULL, (void *)hy_package_get_epoch},
    {"installsize", (getter)get_num, NULL, NULL,
     (void *)hy_package_get_installsize},
    {"buildtime", (getter)get_num, NULL, NULL, (void *)hy_package_get_buildtime},
    {"installtime", (getter)get_num, NULL, NULL,
     (void *)hy_package_get_installtime},
    {"installed", (getter)get_bool, NULL, NULL, (void *)hy_package_installed},
    {"medianr", (getter)get_num, NULL, NULL, (void *)hy_package_get_medianr},
    {"rpmdbid", (getter)get_num, NULL, NULL, (void *)hy_package_get_rpmdbid},
    {"size", (getter)get_num, NULL, NULL, (void *)hy_package_get_size},
    {"conflicts",  (getter)get_reldep, NULL, NULL,
     (void *)hy_package_get_conflicts},
    {"enhances",  (getter)get_reldep, NULL, NULL,
     (void *)hy_package_get_enhances},
    {"obsoletes",  (getter)get_reldep, NULL, NULL,
     (void *)hy_package_get_obsoletes},
    {"provides",  (getter)get_reldep, NULL, NULL,
     (void *)hy_package_get_provides},
    {"recommends",  (getter)get_reldep, NULL, NULL,
     (void *)hy_package_get_recommends},
    {"requires",  (getter)get_reldep, NULL, NULL,
     (void *)hy_package_get_requires},
    {"suggests",  (getter)get_reldep, NULL, NULL,
     (void *)hy_package_get_suggests},
    {"supplements",  (getter)get_reldep, NULL, NULL,
     (void *)hy_package_get_supplements},
    {NULL}			/* sentinel */
};

/* object methods */

static PyObject *
evr_cmp(_PackageObject *self, PyObject *other)
{
    HyPackage pkg2 = packageFromPyObject(other);
    if (pkg2 == NULL)
	return NULL;
    return PyLong_FromLong(hy_package_evr_cmp(self->package, pkg2));
}

static PyObject *
get_delta_from_evr(_PackageObject *self, PyObject *evr_str)
{
    PyObject *tmp_py_str = NULL;
    const char *evr = pycomp_get_string(evr_str, &tmp_py_str);
    if (evr == NULL) {
        Py_XDECREF(tmp_py_str);
        return NULL;
    }
    HyPackageDelta delta_c = hy_package_get_delta_from_evr(self->package, evr);
    Py_XDECREF(tmp_py_str);
    if (delta_c)
	return packageDeltaToPyObject(delta_c);
    Py_RETURN_NONE;
}

static PyObject *
get_advisories(_PackageObject *self, PyObject *args)
{
    int cmp_type;
    HyAdvisoryList advisories;
    PyObject *list;

    if (!PyArg_ParseTuple(args, "i", &cmp_type))
	return NULL;

    advisories = hy_package_get_advisories(self->package, cmp_type);
    list = advisorylist_to_pylist(advisories, self->sack);
    hy_advisorylist_free(advisories);

    return list;
}

static struct PyMethodDef package_methods[] = {
    {"evr_cmp", (PyCFunction)evr_cmp, METH_O, NULL},
    {"get_delta_from_evr", (PyCFunction)get_delta_from_evr, METH_O, NULL},
    {"get_advisories", (PyCFunction)get_advisories, METH_VARARGS, NULL},
    {NULL}                      /* sentinel */
};

PyTypeObject package_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hawkey.Package",		/*tp_name*/
    sizeof(_PackageObject),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    (destructor) package_dealloc, /*tp_dealloc*/
    0,				/*tp_print*/
    0,				/*tp_getattr*/
    0,				/*tp_setattr*/
    0,				/*tp_compare*/
    (reprfunc)package_repr,	/*tp_repr*/
    0,				/*tp_as_number*/
    0,				/*tp_as_sequence*/
    0,				/*tp_as_mapping*/
    (hashfunc)package_hash,	/*tp_hash */
    0,				/*tp_call*/
    (reprfunc)package_str,	/*tp_str*/
    0,				/*tp_getattro*/
    0,				/*tp_setattro*/
    0,				/*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,	/*tp_flags*/
    "Package object",		/* tp_doc */
    0,				/* tp_traverse */
    0,				/* tp_clear */
    (richcmpfunc) package_py_richcompare,	/* tp_richcompare */
    0,				/* tp_weaklistoffset */
    0,				/* tp_iter */
    0,                         	/* tp_iternext */
    package_methods,		/* tp_methods */
    0,				/* tp_members */
    package_getsetters,		/* tp_getset */
    0,				/* tp_base */
    0,				/* tp_dict */
    0,				/* tp_descr_get */
    0,				/* tp_descr_set */
    0,				/* tp_dictoffset */
    (initproc)package_init,	/* tp_init */
    0,				/* tp_alloc */
    package_new,		/* tp_new */
    0,				/* tp_free */
    0,				/* tp_is_gc */
};
