#include <Python.h>

#include "util.h"
#include "src/query.h"
#include "hawkey-pysys.h"
#include "iutil-py.h"
#include "package-py.h"
#include "query-py.h"
#include "sack-py.h"

typedef struct {
    PyObject_HEAD
    Query query;
    PyObject *sack;
} _QueryObject;

/* functions on the type */

static PyObject *
query_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _QueryObject *self = (_QueryObject *)type->tp_alloc(type, 0);
    if (self) {
	self->query = NULL;
	self->sack = NULL;
    }
    return (PyObject *)self;
}

static void
query_dealloc(_QueryObject *self)
{
    if (self->query)
	query_free(self->query);
    Py_XDECREF(self->sack);
    Py_TYPE(self)->tp_free(self);
}

static int
query_init(_QueryObject * self, PyObject *args, PyObject *kwds)
{
    PyObject *sack;
    Sack csack;

    if (!PyArg_ParseTuple(args, "O!", &sack_Type, &sack))
	return -1;
    csack = sackFromPyObject(sack);
    if (csack == NULL)
	return -1;
    self->sack = sack;
    Py_INCREF(self->sack);
    self->query = query_create(csack);
    return 0;
}

/* object methods */

static PyObject *
filter(_QueryObject *self, PyObject *args)
{
    key_t keyname;
    int filtertype;
    PyObject *match;
    const char *cmatch;

    if (!PyArg_ParseTuple(args, "iiO", &keyname, &filtertype, &match))
	return NULL;
    if (keyname == KN_PKG_LATEST || keyname == KN_PKG_UPDATES ||
	keyname == KN_PKG_OBSOLETING) {
	long val;

	if (!PyInt_Check(match) || filtertype != FT_EQ) {
	    PyErr_SetString(PyExc_ValueError, "Invalid boolean filter query.");
	    return NULL;
	}
	val = PyInt_AsLong(match);
	if (keyname == KN_PKG_LATEST)
	    query_filter_latest(self->query, val);
	else if (keyname == KN_PKG_UPDATES)
	    query_filter_updates(self->query, val);
	else
	    query_filter_obsoleting(self->query, val);
	Py_RETURN_NONE;
    }
    if ((cmatch = PyString_AsString(match)) == NULL)
	return NULL;
    query_filter(self->query, keyname, filtertype, cmatch);
    Py_RETURN_NONE;
}

static PyObject *
run(_QueryObject *self, PyObject *unused)
{
    PackageList plist;
    PyObject *list;

    plist = query_run(self->query);
    list = packagelist_to_pylist(plist, self->sack);
    packagelist_free(plist);
    return list;
}

static struct PyMethodDef query_methods[] = {
    {"filter", (PyCFunction)filter, METH_VARARGS,
     NULL},
    {"run", (PyCFunction)run, METH_NOARGS,
     NULL},
    {NULL}                      /* sentinel */
};

PyTypeObject query_Type = {
    PyObject_HEAD_INIT(NULL)
    0,				/*ob_size*/
    "_hawkey.Query",		/*tp_name*/
    sizeof(_QueryObject),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    (destructor) query_dealloc, /*tp_dealloc*/
    0,				/*tp_print*/
    0,				/*tp_getattr*/
    0,				/*tp_setattr*/
    0,				/*tp_compare*/
    0,				/*tp_repr*/
    0,				/*tp_as_number*/
    0,				/*tp_as_sequence*/
    0,				/*tp_as_mapping*/
    0,				/*tp_hash */
    0,				/*tp_call*/
    0,				/*tp_str*/
    0,				/*tp_getattro*/
    0,				/*tp_setattro*/
    0,				/*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,		/*tp_flags*/
    "Query object",		/* tp_doc */
    0,				/* tp_traverse */
    0,				/* tp_clear */
    0,				/* tp_richcompare */
    0,				/* tp_weaklistoffset */
    PyObject_SelfIter,		/* tp_iter */
    0,                         	/* tp_iternext */
    query_methods,		/* tp_methods */
    0,				/* tp_members */
    0,				/* tp_getset */
    0,				/* tp_base */
    0,				/* tp_dict */
    0,				/* tp_descr_get */
    0,				/* tp_descr_set */
    0,				/* tp_dictoffset */
    (initproc)query_init,	/* tp_init */
    0,				/* tp_alloc */
    query_new,			/* tp_new */
    0,				/* tp_free */
    0,				/* tp_is_gc */
};
