#include <Python.h>

// libsolv
#include <solv/util.h>

// hawkey
#include "src/packagelist.h"
#include "src/packageset.h"
#include "src/query.h"
#include "src/reldep.h"

// pyhawkey
#include "exception-py.h"
#include "hawkey-pysys.h"
#include "iutil-py.h"
#include "package-py.h"
#include "query-py.h"
#include "reldep-py.h"
#include "sack-py.h"

typedef struct {
    PyObject_HEAD
    HyQuery query;
    PyObject *sack;
} _QueryObject;

HyQuery
queryFromPyObject(PyObject *o)
{
    if (!PyType_IsSubtype(o->ob_type, &query_Type)) {
	PyErr_SetString(PyExc_TypeError, "Expected a Query object.");
	return NULL;
    }
    return ((_QueryObject *)o)->query;
}

int
query_converter(PyObject *o, HyQuery *query_ptr)
{
    HyQuery query = queryFromPyObject(o);
    if (query == NULL)
	return 0;
    *query_ptr = query;
    return 1;
}

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
	hy_query_free(self->query);
    Py_XDECREF(self->sack);
    Py_TYPE(self)->tp_free(self);
}

static int
query_init(_QueryObject * self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"sack", "query", NULL};
    PyObject *sack;
    PyObject *query;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &sack, &query))
	return -1;

    if (query && sack == Py_None && queryObject_Check(query)) {
	_QueryObject *query_obj = (_QueryObject*)query;
	self->sack = query_obj->sack;
	self->query = hy_query_clone(query_obj->query);
    } else if (sack && query == Py_None && sackObject_Check(sack)) {
	HySack csack = sackFromPyObject(sack);
	assert(csack);
	self->sack = sack;
	self->query = hy_query_create(csack);
    } else {
	const char *msg = "Expected a _hawkey.Sack or a _hawkey.Query object.";
	PyErr_SetString(PyExc_TypeError, msg);
	return -1;
    }
    Py_INCREF(self->sack);
    return 0;
}

/* object methods */

static PyObject *
clear(_QueryObject *self, PyObject *unused)
{
    hy_query_clear(self->query);
    Py_RETURN_NONE;
}

static PyObject *
raise_bad_filter(void)
{
    PyErr_SetString(HyExc_Query, "Invalid filter key or match type.");
    return NULL;
}

static PyObject *
filter(_QueryObject *self, PyObject *args)
{
    key_t keyname;
    int cmp_type;
    PyObject *match;
    const char *cmatch;

    if (!PyArg_ParseTuple(args, "iiO", &keyname, &cmp_type, &match))
	return NULL;
    if (keyname == HY_PKG_DOWNGRADES ||
	keyname == HY_PKG_EMPTY ||
	keyname == HY_PKG_LATEST ||
	keyname == HY_PKG_UPGRADES) {
	long val;

	if (!PyInt_Check(match) || cmp_type != HY_EQ) {
	    PyErr_SetString(HyExc_Value, "Invalid boolean filter query.");
	    return NULL;
	}
	val = PyInt_AsLong(match);
	if (keyname == HY_PKG_EMPTY) {
	    if (!val) {
		PyErr_SetString(HyExc_Value, "Invalid boolean filter query.");
		return NULL;
	    }
	    hy_query_filter_empty(self->query);
	} else if (keyname == HY_PKG_LATEST)
	    hy_query_filter_latest(self->query, val);
	else if (keyname == HY_PKG_DOWNGRADES)
	    hy_query_filter_downgrades(self->query, val);
	else
	    hy_query_filter_upgrades(self->query, val);
	Py_RETURN_NONE;
    }
    if (PyString_Check(match)) {
	cmatch = PyString_AsString(match);
	if (hy_query_filter(self->query, keyname, cmp_type, cmatch))
	    return raise_bad_filter();
	Py_RETURN_NONE;
    }
    if (PyInt_Check(match)) {
	long val = PyInt_AsLong(match);
	if (val > INT_MAX || val < INT_MIN) {
	    PyErr_SetString(HyExc_Value, "Numeric argument out of range.");
	    return NULL;
	}
	if (hy_query_filter_num(self->query, keyname, cmp_type, val))
	    return raise_bad_filter();
	Py_RETURN_NONE;
    }
    if (queryObject_Check(match)) {
	HyQuery target = queryFromPyObject(match);
	HyPackageSet pset = hy_query_run_set(target);
	int ret = hy_query_filter_package_in(self->query, keyname,
					     cmp_type, pset);

	hy_packageset_free(pset);
	if (ret)
	    return raise_bad_filter();
	Py_RETURN_NONE;
    }
    if (reldepObject_Check(match)) {
	HyReldep reldep = reldepFromPyObject(match);
	if (cmp_type != HY_EQ ||
	    hy_query_filter_reldep(self->query, keyname, reldep))
	    return raise_bad_filter();
	Py_RETURN_NONE;
    }
    if (!PySequence_Check(match)) {
	PyErr_SetString(PyExc_TypeError, "Invalid filter match type.");
	return NULL;
    }
    // match is a sequence now:
    switch (keyname) {
    case HY_PKG:
    case HY_PKG_OBSOLETES: {
	HySack sack = sackFromPyObject(self->sack);
	assert(sack);
	HyPackageSet pset = pyseq_to_packageset(match, sack);

	if (pset == NULL)
	    return NULL;
	int ret = hy_query_filter_package_in(self->query, keyname,
					     cmp_type, pset);
	hy_packageset_free(pset);
	if (ret)
	    return raise_bad_filter();

	break;
    }
    case HY_PKG_PROVIDES: {
	HySack sack = sackFromPyObject(self->sack);
	assert(sack);
	HyReldepList reldeplist = pyseq_to_reldeplist(match, sack);
	if (reldeplist == NULL)
	    return NULL;

	int ret = hy_query_filter_reldep_in(self->query, keyname, reldeplist);
	hy_reldeplist_free(reldeplist);
	if (ret)
	    return raise_bad_filter();
	break;
    }
    default: {
	const unsigned count = PySequence_Size(match);
	const char *matches[count + 1];
	matches[count] = NULL;
	for (int i = 0; i < count; ++i) {
	    PyObject *item = PySequence_GetItem(match, i);
	    if (!PyString_Check(item)) {
		PyErr_SetString(PyExc_TypeError, "Invalid filter match value.");
		return NULL;
	    }
	    Py_DECREF(item);
	    matches[i] = PyString_AsString(item);
	}
	if (hy_query_filter_in(self->query, keyname, cmp_type, matches))
	    return raise_bad_filter();
	break;
    }
    }
    Py_RETURN_NONE;
}

static PyObject *
run(_QueryObject *self, PyObject *unused)
{
    HyPackageList plist;
    PyObject *list;

    plist = hy_query_run(self->query);
    list = packagelist_to_pylist(plist, self->sack);
    hy_packagelist_free(plist);
    return list;
}

static struct PyMethodDef query_methods[] = {
    {"clear", (PyCFunction)clear, METH_NOARGS,
     NULL},
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
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,	/*tp_flags*/
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
