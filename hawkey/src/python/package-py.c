#include <Python.h>
#include <stdio.h>

// libsolv
#include <solv/util.h>

// hawkey
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
    self->package = package_create(sack_pool(csack), id);
    return 0;
}

int
package_py_cmp(_PackageObject *self, _PackageObject *other)
{
    long cmp = hy_package_cmp(self->package, other->package);
    if (cmp > 0)
	cmp = 1;
    else if (cmp < 0)
	cmp = -1;
    return cmp;
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
    return PyString_FromString(cstr);
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
    ret = PyString_FromString(cstr);
    solv_free(cstr);
    return ret;
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
    return Py_BuildValue("is#", type, cs, checksum_type2length(type));
}

static PyGetSetDef package_getsetters[] = {
    {"location",  (getter)get_str_alloced, NULL, NULL,
     (void *)hy_package_get_location},
    {"baseurl",  (getter)get_str, NULL, NULL,
     (void *)hy_package_get_baseurl},
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
    {"epoch", (getter)get_num, NULL, NULL, (void *)hy_package_get_epoch},
    {"buildtime", (getter)get_num, NULL, NULL, (void *)hy_package_get_buildtime},
    {"installtime", (getter)get_num, NULL, NULL,
     (void *)hy_package_get_installtime},
    {"medianr", (getter)get_num, NULL, NULL, (void *)hy_package_get_medianr},
    {"rpmdbid", (getter)get_num, NULL, NULL, (void *)hy_package_get_rpmdbid},
    {"size", (getter)get_num, NULL, NULL, (void *)hy_package_get_size},
    {"conflicts",  (getter)get_reldep, NULL, NULL,
     (void *)hy_package_get_conflicts},
    {"obsoletes",  (getter)get_reldep, NULL, NULL,
     (void *)hy_package_get_obsoletes},
    {"provides",  (getter)get_reldep, NULL, NULL,
     (void *)hy_package_get_provides},
    {"requires",  (getter)get_reldep, NULL, NULL,
     (void *)hy_package_get_requires},
    {NULL}			/* sentinel */
};

/* object methods */

static PyObject *
evr_cmp(_PackageObject *self, PyObject *other)
{
    HyPackage pkg2 = packageFromPyObject(other);
    if (pkg2 == NULL)
	return NULL;
    return PyInt_FromLong(hy_package_evr_cmp(self->package, pkg2));
}

static PyObject *
get_delta_from_evr(_PackageObject *self, PyObject *evr_str)
{
    const char *evr = PyString_AsString(evr_str);
    if (evr == NULL)
	return NULL;
    HyPackageDelta delta_c = hy_package_get_delta_from_evr(self->package, evr);
    PyObject *delta = packageDeltaToPyObject(delta_c);
    if (delta)
	return delta;
    Py_RETURN_NONE;
}

static struct PyMethodDef package_methods[] = {
    {"evr_cmp", (PyCFunction)evr_cmp, METH_O, NULL},
    {"get_delta_from_evr", (PyCFunction)get_delta_from_evr, METH_O, NULL},
    {NULL}                      /* sentinel */
};

PyTypeObject package_Type = {
    PyObject_HEAD_INIT(NULL)
    0,				/*ob_size*/
    "_hawkey.Package",		/*tp_name*/
    sizeof(_PackageObject),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    (destructor) package_dealloc, /*tp_dealloc*/
    0,				/*tp_print*/
    0,				/*tp_getattr*/
    0,				/*tp_setattr*/
    (cmpfunc)package_py_cmp,	/*tp_compare*/
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
    0,				/* tp_richcompare */
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
