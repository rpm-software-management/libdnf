#include "Python.h"

// hawkey
#include "src/package_internal.h"
#include "src/sack_internal.h"

// pyhawkey
#include "hawkey-pysys.h"
#include "package-py.h"
#include "repo-py.h"
#include "sack-py.h"

typedef struct {
    PyObject_HEAD
    Sack sack;
    PyObject *custom_package_class;
    PyObject *custom_package_val;
} _SackObject;

PyObject *
new_package(PyObject *sack, Id id)
{
    _SackObject *self;

    if (!sackObject_Check(sack)) {
	PyErr_SetString(PyExc_TypeError, "Expected a _hawkey.Sack object.");
	return NULL;
    }
    self = (_SackObject*)sack;
    PyObject *arglist;
    if (self->custom_package_class || self->custom_package_val) {
	arglist = Py_BuildValue("(Oi)O", sack, id, self->custom_package_val);
    } else {
	arglist = Py_BuildValue("((Oi))", sack, id);
    }
    if (arglist == NULL)
	return NULL;
    PyObject *package;
    if (self->custom_package_class) {
	package = PyObject_CallObject(self->custom_package_class, arglist);
    } else {
	package = PyObject_CallObject((PyObject*)&package_Type, arglist);
    }
    return package;
}

Sack
sackFromPyObject(PyObject *o)
{
    if (!sackObject_Check(o)) {
	PyErr_SetString(PyExc_TypeError, "Expected a _hawkey.Sack object.");
	return NULL;
    }
    return ((_SackObject *)o)->sack;
}

/* functions on the type */

static void
sack_dealloc(_SackObject *o)
{
    if (o->sack)
	sack_free(o->sack);
    Py_TYPE(o)->tp_free(o);
}

static PyObject *
sack_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _SackObject *self = (_SackObject *)type->tp_alloc(type, 0);

    if (self) {
	self->sack = sack_create();
	if (self->sack == NULL) {
	    Py_DECREF(self);
	    return NULL;
	}
    }
    self->custom_package_class = NULL;
    self->custom_package_val = NULL;
    return (PyObject *)self;
}

static int
sack_init(_SackObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *custom_class = NULL;
    PyObject *custom_val = NULL;

    if (!PyArg_ParseTuple(args, "|OO", &custom_class, &custom_val))
	return -1;
    if (custom_class && custom_class != Py_None) {
	if (!PyType_Check(custom_class)) {
	    PyErr_SetString(PyExc_TypeError, "Expected a class object.");
	    return -1;
	}
	Py_INCREF(custom_class);
	self->custom_package_class = custom_class;
    }
    if (custom_val && custom_val != Py_None) {
	Py_INCREF(custom_val);
	self->custom_package_val = custom_val;

    }
    return 0;
}

/* getsetters */

static PyObject *
get_nsolvables(_SackObject *self, void *unused)
{
    return PyInt_FromLong(sack_pool(self->sack)->nsolvables);
}


static PyGetSetDef sack_getsetters[] = {
    {"nsolvables", (getter)get_nsolvables, NULL, NULL, NULL},
    {NULL}			/* sentinel */
};

/* object methods */

static PyObject *
create_cmdline_repo(_SackObject *self, PyObject *unused)
{
    sack_create_cmdline_repo(self->sack);
    Py_RETURN_NONE;
}

static PyObject *
create_package(_SackObject *self, PyObject *solvable_id)
{
    Id id  = PyInt_AsLong(solvable_id);
    if (id <= 0) {
	PyErr_SetString(PyExc_TypeError, "Expected a positive integer.");
	return NULL;
    }
    return new_package((PyObject*)self, id);
}

static PyObject *
add_cmdline_rpm(_SackObject *self, PyObject *fn_obj)
{
    Package cpkg;
    PyObject *pkg;
    const char *fn = PyString_AsString(fn_obj);

    if (fn == NULL)
	return NULL;
    cpkg = sack_add_cmdline_rpm(self->sack, fn);
    if (cpkg == NULL) {
	PyErr_SetString(PyExc_IOError, "Can not load an .rpm file");
	return NULL;
    }
    pkg = new_package((PyObject*)self, package_id(cpkg));
    package_free(cpkg);
    return pkg;
}

static PyObject *
load_rpm_repo(_SackObject *self, PyObject *unused)
{
    sack_load_rpm_repo(self->sack);
    Py_RETURN_NONE;
}

static PyObject *
load_yum_repo(_SackObject *self, PyObject *repo)
{
    FRepo frepo = frepoFromPyObject(repo);
    if (frepo == NULL)
	return NULL;
    sack_load_yum_repo(self->sack, frepo);

    Py_RETURN_NONE;
}

static PyObject *
write_all_repos(_SackObject *self, PyObject *unused)
{
    return PyInt_FromLong(sack_write_all_repos(self->sack));
}

static struct PyMethodDef sack_methods[] = {
    {"create_cmdline_repo", (PyCFunction)create_cmdline_repo, METH_NOARGS,
     NULL},
    {"create_package", (PyCFunction)create_package, METH_O,
     NULL},
    {"add_cmdline_rpm", (PyCFunction)add_cmdline_rpm, METH_O,
     NULL},
    {"load_rpm_repo", (PyCFunction)load_rpm_repo, METH_NOARGS,
     NULL},
    {"load_yum_repo", (PyCFunction)load_yum_repo, METH_O,
     NULL},
    {"write_all_repos", (PyCFunction)write_all_repos, METH_NOARGS,
     NULL},
    {NULL}                      /* sentinel */
};

PyTypeObject sack_Type = {
    PyObject_HEAD_INIT(NULL)
    0,				/*ob_size*/
    "_hawkey.Sack",		/*tp_name*/
    sizeof(_SackObject),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    (destructor) sack_dealloc,  /*tp_dealloc*/
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
    "Sack object",		/* tp_doc */
    0,				/* tp_traverse */
    0,				/* tp_clear */
    0,				/* tp_richcompare */
    0,				/* tp_weaklistoffset */
    PyObject_SelfIter,		/* tp_iter */
    0,                         	/* tp_iternext */
    sack_methods,		/* tp_methods */
    0,				/* tp_members */
    sack_getsetters,		/* tp_getset */
    0,				/* tp_base */
    0,				/* tp_dict */
    0,				/* tp_descr_get */
    0,				/* tp_descr_set */
    0,				/* tp_dictoffset */
    (initproc)sack_init,	/* tp_init */
    0,				/* tp_alloc */
    sack_new,			/* tp_new */
    0,				/* tp_free */
    0,				/* tp_is_gc */
};
